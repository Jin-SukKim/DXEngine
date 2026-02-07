#include "pch.h"
#include "ParticleManager.h"
#include "ParticleLoader.h"
#include "RenderBase.h"
#include "RenderModule.h"
#include "ModelManager.h"
#include "MaterialModule.h"
#include "MaterialSystem.h"
#include "TextureManager.h"
#include "ScopedTimer.h"

namespace DE {
	void ParticleManager::Initialize()
	{
		m_memoryPool = std::make_unique<ParticleMemoryPool>();
		m_memoryPool->Initialize(10000000, 10000, 10000);
	}

	void ParticleManager::Update(const float& dt)
	{
		// [Added] Measure Total Update Time
		ScopedTimer timer([&](float t) { UpdateMetric(m_runtimeProfile.update, t); });

		constexpr float DEFRAG_THRESHOLD = 0.2f;
		if (m_memoryPool->GetFragmentationRatio() >= DEFRAG_THRESHOLD) {
			// �� �籸�� �ð� ���� (Legacy Metric - Cumulative Average)
			auto start = std::chrono::high_resolution_clock::now();
			Defragment(); // Defragment internally also measures for RuntimeProfile

			auto end = std::chrono::high_resolution_clock::now();
			float elapsed = std::chrono::duration<float, std::milli>(end - start).count();

			m_rebuildCount++;
			m_totalRebuildTime += elapsed;
			m_avgRebuildTime = m_totalRebuildTime / m_rebuildCount;
		}

		// [Added] Measure Prepare & Upload Phase
		{
			ScopedTimer tPrepare([&](float t) { UpdateMetric(m_runtimeProfile.update_prepare, t); });

			// (A) Setup
			{
				ScopedTimer tSetup([&](float t) { UpdateMetric(m_runtimeProfile.update_prepare_setup, t); });
				m_memoryPool->ClearWriteCount();
				m_memoryPool->BindCompute();
			}

			// (B) CPU PreUpdate
			{
				ScopedTimer tCpu([&](float t) { UpdateMetric(m_runtimeProfile.update_prepare_cpu, t); });
				for (auto* system : m_activeSystems) {
					if (system) {
						system->PreUpdate(dt, m_memoryPool->GetFrameConsts().GetCpu());
					}
				}
			}

			// (C) GPU Upload
			{
				ScopedTimer tUpload([&](float t) { UpdateMetric(m_runtimeProfile.update_prepare_upload, t); });
				m_memoryPool->UploadFrameConsts();
				m_memoryPool->UploadEmitterIDs();
			}
		}

		// [Added] Measure Args Update
		{
			ScopedTimer tArgs([&](float t) { UpdateMetric(m_runtimeProfile.update_args, t); });
			m_memoryPool->UpdateArgs();
		}

		// [Added] Measure Dispatch Logic
		{
			ScopedTimer tDispatch([&](float t) { UpdateMetric(m_runtimeProfile.update_dispatch, t); });
			m_memoryPool->ExcuteParticleLogic();
			for (auto* system : m_activeSystems) {
				if (system) {
					m_memoryPool->BindMeshConsts(system->GetPoolHandle().systemSlot);
					system->Update(dt);
				}
			}
			m_memoryPool->UnbindCompute();
		}

		// Compute ��, SwapBuffer ���� Read ������ ����ȭ
		if (m_needsSyncReadOffset) {
			SyncReadOffsets();
			m_needsSyncReadOffset = false;
		}

		// [Added] Measure Swap Buffer
		if (m_memoryPool) {
			ScopedTimer tSwap([&](float t) { UpdateMetric(m_runtimeProfile.update_swap, t); });
			m_memoryPool->SwapBuffer();
		}
	}

	void ParticleManager::Render()
	{
		ScopedTimer timer([&](float t) { UpdateMetric(m_runtimeProfile.render, t); });

		if (m_activeSystems.empty()) return;

		m_memoryPool->BindRender();
		m_memoryPool->UpdateRenderArgs();

		// 1. 모든 활성 emitter 수집
		std::vector<EmitterRenderInfo> allInfos;
		CollectEmitterRenderInfos(allInfos);

		// 2. 배치 그룹 구성
		std::vector<BatchGroup> batches;
		std::vector<EmitterRenderInfo> unbatched;
		BuildBatchGroups(allInfos, batches, unbatched);

		// 3. 배치 렌더링 (배칭 가능한 그룹)
		RenderBatched(batches);

		// 4. 비배치 렌더링 (기존 방식)
		RenderUnbatched(unbatched);

		m_memoryPool->UnbindRender();
		GET_SINGLE(RenderBase)->SetPipelineState(RenderBase::graphicsCommon.basic.solidPSO);

		// 통계 갱신
		m_lastBatchCount = static_cast<UINT>(batches.size());
		m_lastDrawCallCount = static_cast<UINT>(batches.size() + unbatched.size());
	}

	void ParticleManager::CollectEmitterRenderInfos(std::vector<EmitterRenderInfo>& outInfos)
	{
		for (auto* system : m_activeSystems)
		{
			if (!system || system->IsStopped()) continue;

			const PoolHandle& handle = system->GetPoolHandle();
			if (!handle.IsActive()) continue;

			// 시스템의 모든 emitter를 순회
			auto collectFromEmitters = [&](auto& emitters, bool isUniquePtr) {
				for (auto& emitterRef : emitters)
				{
					ParticleEmitter* emitter = nullptr;
					if constexpr (std::is_same_v<std::decay_t<decltype(emitterRef)>, std::unique_ptr<ParticleEmitter>>)
						emitter = emitterRef.get();
					else
						emitter = emitterRef;

					if (!emitter || emitter->IsCompleted()) continue;

					RenderModule* renderMod = emitter->GetRenderModule();
					if (!renderMod) continue;

					UINT localID = emitter->GetEmitterID();
					UINT globalID = handle.emitterIDs[localID];

					EmitterRenderInfo info;
					info.emitter = emitter;
					info.system = system;
					info.globalEmitterID = globalID;

					info.batchKey.isMesh = renderMod->IsMesh();
					info.batchKey.blendMode = static_cast<int>(renderMod->blendMode);
					info.batchKey.textureMode = renderMod->GetTextureMode();
					info.batchKey.singleTextureIdx = renderMod->GetSingleTextureIdx();
					info.batchKey.modelIdx = renderMod->GetModelIndex();

					// useSorting이나 Material 모드는 배칭 불가
					bool useSorting = (m_memoryPool->GetConsts().GetCpu()[globalID].render.useSorting != 0);
					info.canBatch = renderMod->IsBatchable() && !useSorting;

					outInfos.push_back(info);
				}
			};

			// main emitters (unique_ptr)
			auto& mainEmitters = system->GetMainEmitters();
			for (size_t i = 0; i < mainEmitters.size(); ++i)
			{
				ParticleEmitter* emitter = mainEmitters[i].get();
				if (!emitter || emitter->IsCompleted()) continue;

				RenderModule* renderMod = emitter->GetRenderModule();
				if (!renderMod) continue;

				// GetEmitterID()는 이미 globalID를 반환함
				UINT globalID = emitter->GetEmitterID();

				EmitterRenderInfo info;
				info.emitter = emitter;
				info.system = system;
				info.globalEmitterID = globalID;

				info.batchKey.isMesh = renderMod->IsMesh();
				info.batchKey.blendMode = static_cast<int>(renderMod->blendMode);
				info.batchKey.textureMode = renderMod->GetTextureMode();
				info.batchKey.singleTextureIdx = renderMod->GetSingleTextureIdx();
				info.batchKey.modelIdx = renderMod->GetModelIndex();

				bool useSorting = (m_memoryPool->GetConsts().GetCpu()[globalID].render.useSorting != 0);
				info.canBatch = renderMod->IsBatchable() && !useSorting;

				outInfos.push_back(info);
			}

			// active sub emitters (raw pointers)
			auto& subEmitters = system->GetActiveSubEmitters();
			for (auto* emitter : subEmitters)
			{
				if (!emitter || emitter->IsCompleted()) continue;

				RenderModule* renderMod = emitter->GetRenderModule();
				if (!renderMod) continue;

				// GetEmitterID()는 이미 globalID를 반환함
				UINT globalID = emitter->GetEmitterID();

				EmitterRenderInfo info;
				info.emitter = emitter;
				info.system = system;
				info.globalEmitterID = globalID;

				info.batchKey.isMesh = renderMod->IsMesh();
				info.batchKey.blendMode = static_cast<int>(renderMod->blendMode);
				info.batchKey.textureMode = renderMod->GetTextureMode();
				info.batchKey.singleTextureIdx = renderMod->GetSingleTextureIdx();
				info.batchKey.modelIdx = renderMod->GetModelIndex();

				bool useSorting = (m_memoryPool->GetConsts().GetCpu()[globalID].render.useSorting != 0);
				info.canBatch = renderMod->IsBatchable() && !useSorting;

				outInfos.push_back(info);
			}
		}
	}

	void ParticleManager::BuildBatchGroups(
		const std::vector<EmitterRenderInfo>& infos,
		std::vector<BatchGroup>& outBatches,
		std::vector<EmitterRenderInfo>& outUnbatched)
	{
		// BatchKey별로 그룹핑
		std::map<BatchKey, std::vector<UINT>> keyToEmitters;

		for (const auto& info : infos)
		{
			if (!info.canBatch) {
				outUnbatched.push_back(info);
				continue;
			}
			keyToEmitters[info.batchKey].push_back(info.globalEmitterID);
		}

		// 2개 이상 emitter가 같은 키를 공유하면 배치, 1개면 비배치
		UINT totalBatchedEmitters = 0;
		for (auto& [key, emitterIDs] : keyToEmitters)
		{
			// 배치 가능 조건: 2개 이상 && 배치 수 제한 && 총 이미터 수 제한
			bool canBatch = emitterIDs.size() >= 2
				&& outBatches.size() < MAX_BATCH_GROUPS
				&& (totalBatchedEmitters + emitterIDs.size()) <= MAX_BATCH_EMITTERS;

			if (canBatch)
			{
				BatchGroup group;
				group.key = key;
				group.globalEmitterIDs = std::move(emitterIDs);
				totalBatchedEmitters += static_cast<UINT>(group.globalEmitterIDs.size());
				outBatches.push_back(std::move(group));
			}
			else
			{
				// 배치 불가능하면 비배치로 넘김
				for (UINT eid : emitterIDs)
				{
					for (const auto& info : infos)
					{
						if (info.globalEmitterID == eid)
						{
							outUnbatched.push_back(info);
							break;
						}
					}
				}
			}
		}
	}

	void ParticleManager::RenderBatched(const std::vector<BatchGroup>& batches)
	{
		if (batches.empty()) return;

		UINT batchCount = static_cast<UINT>(batches.size());

		// 1. 배치 데이터 GPU 업로드
		m_memoryPool->UploadBatchData(batches);

		// 2. BatchArgsUpdateCS 디스패치 (파티클 수 & prefix sum 계산)
		m_memoryPool->UpdateBatchArgs(batchCount);

		auto context = GET_SINGLE(RenderBase)->GetContext();

		// 3. 배치별 렌더링
		BlendMode lastBlend = BlendMode::Additive;
		bool lastIsMesh = false;
		bool psoSet = false;

		for (UINT b = 0; b < batchCount; ++b)
		{
			const BatchGroup& batch = batches[b];
			bool isMesh = batch.key.isMesh;
			BlendMode blend = static_cast<BlendMode>(batch.key.blendMode);

			// PSO 설정 (상태 변경 최소화)
			if (!psoSet || isMesh != lastIsMesh)
			{
				if (isMesh)
					GET_SINGLE(RenderBase)->SetPipelineState(RenderBase::graphicsCommon.particle.batchedMeshPSO);
				else
					GET_SINGLE(RenderBase)->SetPipelineState(RenderBase::graphicsCommon.particle.batchedAnimPSO);
				lastIsMesh = isMesh;
				psoSet = true;
			}

			// BlendState 설정
			if (!psoSet || blend != lastBlend)
			{
				ID3D11BlendState* bs = nullptr;
				switch (blend)
				{
				case BlendMode::Additive:
					bs = RenderBase::graphicsCommon.accumulateBS.Get();
					break;
				case BlendMode::AlphaBlend:
					bs = RenderBase::graphicsCommon.alphaBS.Get();
					break;
				case BlendMode::Opaque:
					bs = nullptr;
					break;
				}
				context->OMSetBlendState(bs, RenderBase::graphicsCommon.particle.animPSO.blendFactor, 0xffffffff);
				lastBlend = blend;
			}

			// 배치 상수 바인딩
			m_memoryPool->BindBatchRender(b);

			// SingleTexture 모드라면 텍스처 바인딩
			if (batch.key.textureMode == 1 && batch.key.singleTextureIdx >= 0)
			{
				ID3D11ShaderResourceView* texSRV = TextureManager::Get().GetTextureSRV(batch.key.singleTextureIdx);
				context->PSSetShaderResources(0, 1, &texSRV);
			}

			// Draw call
			if (isMesh)
			{
				// Mesh 배치: 같은 모델 공유
				Model* model = ModelManager::Get().GetModel(batch.key.modelIdx);
				if (model && !model->meshes.empty())
				{
					auto& mesh = model->meshes[0];
					MaterialSystem::Get().BindMaterial(0);
					context->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &mesh.stride, &mesh.offset);
					context->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
					context->DrawIndexedInstancedIndirect(
						m_memoryPool->GetBatchMeshArgs().GetBuffer(),
						m_memoryPool->GetBatchMeshArgsOffset(b));
				}
			}
			else
			{
				// Billboard 배치
				context->DrawInstancedIndirect(
					m_memoryPool->GetBatchBillboardArgs().GetBuffer(),
					m_memoryPool->GetBatchBillboardArgsOffset(b));
			}
		}

		m_memoryPool->UnbindBatchRender();
	}

	void ParticleManager::RenderUnbatched(const std::vector<EmitterRenderInfo>& unbatched)
	{
		if (unbatched.empty()) return;

		// 상태 정렬: (isMesh, blendMode, textureMode) 기준으로 정렬하여 상태 전환 최소화
		std::vector<const EmitterRenderInfo*> sorted;
		sorted.reserve(unbatched.size());
		for (const auto& info : unbatched)
			sorted.push_back(&info);

		std::sort(sorted.begin(), sorted.end(), [](const EmitterRenderInfo* a, const EmitterRenderInfo* b) {
			return a->batchKey < b->batchKey;
		});

		// 기존 방식으로 렌더링
		for (const auto* info : sorted)
		{
			m_memoryPool->BindMeshConsts(info->system->GetPoolHandle().systemSlot);

			auto context = GET_SINGLE(RenderBase)->GetContext();
			context->VSSetConstantBuffers(6, 1, info->system->GetMeshConstsAddress());

			const PoolHandle& handle = info->system->GetPoolHandle();
			UINT localID = info->emitter->GetEmitterID();
			UINT globalID = handle.emitterIDs[localID];

			m_memoryPool->BindEmitterID(globalID);

			info->emitter->Render(
				{ m_memoryPool->GetBillboardArgs().GetBuffer(),
				  info->system->GetBillboardArgsOffset(globalID) },
				{ m_memoryPool->GetMeshArgs().GetBuffer(),
				  info->system->GetMeshArgsOffset(globalID) }
			);
		}
	}

	void ParticleManager::RegisterActiveSystem(ParticleSystem* system)
	{
		if (!system) return;
		auto it = std::find(m_activeSystems.begin(), m_activeSystems.end(), system);
		if (it == m_activeSystems.end()) {
			m_activeSystems.push_back(system);
		}
	}

	void ParticleManager::UnregisterActiveSystem(ParticleSystem* system)
	{
		if (!system) return;
		auto it = std::find(m_activeSystems.begin(), m_activeSystems.end(), system);
		if (it != m_activeSystems.end()) {
			m_activeSystems.erase(it);
		}
	}

	ParticleSystem* ParticleManager::CreateSystem(const std::wstring& path)
	{
		ParticleSystem* prototype = nullptr;

		// 1. Prototype Load (Cache üũ)
		auto prototypeIt = m_prototypes.find(path);
		if (prototypeIt != m_prototypes.end())
		{
			prototype = prototypeIt->second.get();
		}
		else
		{
			auto newSystem = ParticleLoader::Load<ParticleSystem>(path);
			if (!newSystem) return nullptr;

			newSystem->Initialize();
			prototype = newSystem.get();
			m_prototypes[path] = std::move(newSystem);
		}

		// 2. Clone Instance
		auto cloned = std::make_unique<ParticleSystem>(*prototype);
		ParticleSystem* clonedPtr = cloned.get();

		// 3. CPU Initialize & Memory Allocation
		ParticleInitializer initialData;
		clonedPtr->InitializeCPU(initialData);

		UINT spawnPosCount = static_cast<UINT>(initialData.spawnPositions.size());
		UINT particleCount = clonedPtr->GetTotalParticleCount();
		UINT emitterCount = clonedPtr->GetMaxEmitterCount();

		PoolHandle handle = RequestAllocation(particleCount, emitterCount, spawnPosCount);

		if (!handle.IsActive())
		{
			return nullptr;
		}

		clonedPtr->SetPoolHandle(handle);

		// 4. GPU Resource Upload
		if (!initialData.spawnPositions.empty() && handle.spawnPosOffset != UINT_MAX)
		{
			m_memoryPool->UploadSpawnPositions(handle.spawnPosOffset, initialData.spawnPositions);
		}

		clonedPtr->InitializeGPU(initialData,
			m_memoryPool->GetDispatchArgs(),
			m_memoryPool->GetBillboardArgs(),
			m_memoryPool->GetMeshArgs());

		UploadEmitterIDs(clonedPtr, clonedPtr->GetInitialData());
		m_memoryPool->UploadConsts(handle.emitterIDs, initialData.consts);
		m_memoryPool->UpdateFrameConsts(handle.emitterIDs, initialData.frameConsts);

		// 5. Finalize & Registration
		clonedPtr->Initialize(initialData);
		clonedPtr->OnSpawn();

		m_instances.push_back(std::move(cloned));
		RegisterActiveSystem(clonedPtr);

		return clonedPtr;
	}

	void ParticleManager::DestroyInstance(ParticleSystem* system)
	{
		// [Added] Measure Destroy Time
		ScopedTimer timer([&](float t) { UpdateMetric(m_runtimeProfile.destroy, t); });

		if (!system) return;

		const PoolHandle& handle = system->GetPoolHandle();
		if (handle.IsActive()) {
			m_memoryPool->Free(handle);
		}

		UnregisterActiveSystem(system);

		auto it = std::find_if(m_instances.begin(), m_instances.end(),
			[system](const std::unique_ptr<ParticleSystem>& ptr) {
				return ptr.get() == system;
			});

		if (it != m_instances.end()) {
			m_instances.erase(it);
		}
	}

	void ParticleManager::BindEmitterID(UINT globalSlotIndex)
	{
		m_memoryPool->BindEmitterID(globalSlotIndex);
	}

	void ParticleManager::RenderMemoryPoolGUI()
	{
		if (!m_memoryPool) return;

		if (ImGui::Begin("Particle Memory Pool Status"))
		{
			if (ImGui::CollapsingHeader("Stats Overview", ImGuiTreeNodeFlags_DefaultOpen))
			{
				UINT totalBlocks = m_memoryPool->GetTotalBlockCount();
				UINT usedBlocks = m_memoryPool->GetUsedBlockCount();
				float usage = (totalBlocks > 0) ? (float)usedBlocks / totalBlocks : 0.0f;

				ImGui::Text("Particle Blocks: %d / %d (Size: %d)", usedBlocks, totalBlocks, m_memoryPool->GetBlockSize());
				ImGui::ProgressBar(usage, ImVec2(-1, 0), "Particle Memory Usage");

				ImGui::Text("Max Total Particles Count : %d", usedBlocks * m_memoryPool->GetBlockSize());

				UINT totalEmitters = m_memoryPool->GetTotalEmitterSlots();
				UINT usedEmitters = m_memoryPool->GetUsedEmitterSlots();
				float emitterUsage = (totalEmitters > 0) ? (float)usedEmitters / totalEmitters : 0.0f;

				ImGui::Text("Emitter Slots: %d / %d", usedEmitters, totalEmitters);
				ImGui::ProgressBar(emitterUsage, ImVec2(-1, 0), "Emitter Slot Usage");

				UINT totalSystems = m_memoryPool->GetTotalSystemSlots();
				UINT usedSystems = m_memoryPool->GetUsedSystemSlots();
				ImGui::Text("Active Systems: %d / %d", usedSystems, totalSystems);

				// �� ����ȭ ����
				float fragRatio = m_memoryPool->GetFragmentationRatio();
				ImVec4 fragColor = (fragRatio < 0.2f) ? ImVec4(0, 1, 0, 1) :
					(fragRatio < 0.5f) ? ImVec4(1, 1, 0, 1) : ImVec4(1, 0, 0, 1);
				ImGui::TextColored(fragColor, "Fragmentation: %.1f%%", fragRatio * 100.0f);
			}

			// �� ���� ��Ʈ�� ���� �߰�
			if (ImGui::CollapsingHeader("Performance Metrics", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Text("Rebuild Count: %d", m_rebuildCount);
				ImGui::Text("Avg Rebuild Time: %.3f ms", m_avgRebuildTime);
				ImGui::Text("Total Rebuild Time: %.3f ms", m_totalRebuildTime);

				if (ImGui::Button("Reset Metrics")) {
					m_rebuildCount = 0;
					m_avgRebuildTime = 0.0f;
					m_totalRebuildTime = 0.0f;
				}
			}

			// 2. ���� �ð�ȭ (Grid Visualizer)
			// ���: ��� ��, ȸ��: �� ����
			if (ImGui::CollapsingHeader("Block Map (Visualizer)", ImGuiTreeNodeFlags_DefaultOpen))
			{
				UINT totalBlocks = m_memoryPool->GetTotalBlockCount();
				std::vector<std::string> blockOwners(totalBlocks, "Free");

				const auto& table = m_memoryPool->GetParticleBlockTable();
				int columns = 32; // �� �ٿ� ������ ���� ����
				float cellSize = 10.0f;
				float spacing = 2.0f;

				ImVec2 p = ImGui::GetCursorScreenPos();
				float startX = p.x;
				float startY = p.y;

				for (size_t i = 0; i < table.size(); ++i)
				{
					float x = startX + (i % columns) * (cellSize + spacing);
					float y = startY + (i / columns) * (cellSize + spacing);

					ImU32 color = table[i] ? IM_COL32(50, 205, 50, 255) : IM_COL32(50, 50, 50, 255); // Green vs Gray

					ImGui::GetWindowDrawList()->AddRectFilled(
						ImVec2(x, y),
						ImVec2(x + cellSize, y + cellSize),
						color
					);

					// ���콺 ���� �� ���� ��ȣ ����
					if (ImGui::IsMouseHoveringRect(ImVec2(x, y), ImVec2(x + cellSize, y + cellSize)))
					{
						ImGui::BeginTooltip();
						// [����] ������ �̸����� ���
						ImGui::Text("Block ID: %llu", i);
						ImGui::TextColored(table[i] ? ImVec4(0, 1, 0, 1) : ImVec4(0.5, 0.5, 0.5, 1),
							"Status: %s", table[i] ? "Used" : "Free");

						if (table[i]) {
							ImGui::Text("Owner: %s", blockOwners[i].c_str());
						}
						ImGui::EndTooltip();
					}
				}

				// �׸��� ���̸�ŭ Ŀ�� �̵�
				float totalHeight = ((table.size() + columns - 1) / columns) * (cellSize + spacing);
				ImGui::Dummy(ImVec2(0, totalHeight));
			}
		}
		ImGui::End();

		// [Added] Particle Manager Performance Window
		if (ImGui::Begin("Particle Manager Performance"))
		{
			if (ImGui::CollapsingHeader("Runtime Execution Metrics", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (ImGui::BeginTable("RuntimeMetricsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
				{
					ImGui::TableSetupColumn("Function / Component");
					ImGui::TableSetupColumn("Avg Time (ms)");
					ImGui::TableHeadersRow();

					auto AddRow = [](const char* name, float val, ImVec4 color = ImVec4(1, 1, 1, 1)) {
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0); ImGui::Text("%s", name);
						ImGui::TableSetColumnIndex(1); ImGui::TextColored(color, "%.4f ms", val);
					};

					AddRow("Update (Total)", m_runtimeProfile.update, ImVec4(1.0f, 0.6f, 0.0f, 1.0f));

					ImVec4 subColor = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
					AddRow("  - Prepare & Upload (Total)", m_runtimeProfile.update_prepare, subColor);

					ImVec4 detailColor = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
					AddRow("      > Setup (Bind)", m_runtimeProfile.update_prepare_setup, detailColor);
					AddRow("      > CPU PreUpdate", m_runtimeProfile.update_prepare_cpu, detailColor);
					AddRow("      > GPU Upload", m_runtimeProfile.update_prepare_upload, detailColor);

					AddRow("  - Update Args", m_runtimeProfile.update_args, subColor);
					AddRow("  - Dispatch Logic", m_runtimeProfile.update_dispatch, subColor);
					AddRow("  - Swap Buffer", m_runtimeProfile.update_swap, subColor);

					ImGui::TableNextRow();

					AddRow("Render (Total)", m_runtimeProfile.render, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
					AddRow("  - Batched Draw Calls", static_cast<float>(m_lastBatchCount), subColor);
					AddRow("  - Total Draw Calls", static_cast<float>(m_lastDrawCallCount), subColor);
					AddRow("DestroyInstance", m_runtimeProfile.destroy);
					AddRow("Defragment (Check+Run)", m_runtimeProfile.defrag, (m_runtimeProfile.defrag > 0.1f) ? ImVec4(1, 0, 0, 1) : ImVec4(1, 1, 1, 1));

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::TextDisabled("--- Internal Helpers ---");

					AddRow("  RequestAllocation", m_runtimeProfile.requestAlloc);
					AddRow("  UploadEmitterIDs", m_runtimeProfile.uploadIDs);
					AddRow("  RecalculateOffsets", m_runtimeProfile.recalculateOffsets);
					AddRow("  SyncReadOffsets", m_runtimeProfile.syncReadOffsets);

					ImGui::EndTable();
				}

				if (ImGui::Button("Reset All Runtime Metrics", ImVec2(-1, 0))) {
					m_runtimeProfile.Reset();
				}
			}
		}
		ImGui::End();
	}

	PoolHandle ParticleManager::RequestAllocation(UINT particleCount, UINT emitterCount, UINT spawnPosCount)
	{
		// [Added] Measure Allocation Time
		ScopedTimer timer([&](float t) { UpdateMetric(m_runtimeProfile.requestAlloc, t); });

		PoolHandle handle = m_memoryPool->Allocate(particleCount, emitterCount, spawnPosCount);

		// �Ҵ� ���� �� ���� ������ Defragment ���� (��� ���� X)
		if (!handle.IsActive()) {
			m_needsDefragment = true;
		}

		return handle;
	}

	void ParticleManager::UploadEmitterIDs(ParticleSystem* system, const ParticleInitializer& initialData)
	{
		// [Added] Measure ID Upload Time
		ScopedTimer timer([&](float t) { UpdateMetric(m_runtimeProfile.uploadIDs, t); });

		const PoolHandle& handle = system->GetPoolHandle();

		for (size_t i = 0; i < initialData.emitterIDs.size(); ++i) {
			EmitterID eID = initialData.emitterIDs[i];

			eID.emitterID = handle.emitterIDs[i];
			eID.readParticleOffset = handle.particleOffset + initialData.emitterIDs[i].readParticleOffset;
			eID.writeParticleOffset = handle.particleOffset + initialData.emitterIDs[i].writeParticleOffset;

			// spawnPos�� ����ϴ� emitter�� ������ ����
			if (handle.spawnPosOffset != UINT_MAX && eID.spawnPosOffset != UINT_MAX) {
				eID.spawnPosOffset += handle.spawnPosOffset;
			}

			m_memoryPool->UpdateEmitterID(handle.emitterIDs[i], eID);
		}
	}

	void ParticleManager::UpdateMeshConsts(UINT systemSlot, const MeshConstants& data)
	{
		ParticleMeshConsts pmConsts;
		pmConsts.world = data.world;
		pmConsts.worldIT = data.worldIT;
		pmConsts.vertexCount = 0;
		pmConsts.indexCount = 0;

		m_memoryPool->UpdateMeshConsts(systemSlot, pmConsts);
	}

	void ParticleManager::BindMeshConsts(UINT systemSlot)
	{
		m_memoryPool->BindMeshConsts(systemSlot);
	}

	void ParticleManager::Defragment()
	{
		// [Added] Measure Defragment Time
		ScopedTimer timer([&](float t) { UpdateMetric(m_runtimeProfile.defrag, t); });

		if (m_activeSystems.empty()) return;

		std::vector<PoolHandle> activeHandles;
		for (auto* system : m_activeSystems) {
			if (system && system->GetPoolHandle().IsActive()) {
				activeHandles.push_back(system->GetPoolHandle());
			}
		}

		std::vector<UINT> newOffsets = m_memoryPool->Defragment(activeHandles);

		size_t idx = 0;
		for (auto* system : m_activeSystems) {
			if (!system || !system->GetPoolHandle().IsActive()) continue;

			PoolHandle& handle = system->GetPoolHandle();
			UINT newOffset = newOffsets[idx++];

			if (handle.particleOffset != newOffset) {
				RecalculateEmitterOffsets(system, newOffset);
			}
		}

		m_needsSyncReadOffset = true;  // �̹� ������ Compute �� ����ȭ
	}

	void ParticleManager::RecalculateEmitterOffsets(ParticleSystem* system, UINT newParticleOffset)
	{
		// [Added] Measure Offset Recalculation Time
		ScopedTimer timer([&](float t) { UpdateMetric(m_runtimeProfile.recalculateOffsets, t); });

		PoolHandle& handle = system->GetPoolHandle();
		const ParticleInitializer& initialData = system->GetInitialData();

		for (size_t i = 0; i < handle.emitterIDs.size(); ++i) {
			// writeParticleOffset�� �� ��ġ�� ����
			UINT globalEmitterID = handle.emitterIDs[i];
			UINT localOffset = initialData.emitterIDs[i].writeParticleOffset;

			m_memoryPool->UpdateWriteOffset(globalEmitterID, newParticleOffset + localOffset);
		}

		handle.particleOffset = newParticleOffset;
	}

	void ParticleManager::SyncReadOffsets()
	{
		// [Added] Measure Sync Time
		ScopedTimer timer([&](float t) { UpdateMetric(m_runtimeProfile.syncReadOffsets, t); });

		for (auto* system : m_activeSystems) {
			if (!system || !system->GetPoolHandle().IsActive()) continue;

			const PoolHandle& handle = system->GetPoolHandle();
			for (UINT emitterID : handle.emitterIDs) {
				m_memoryPool->SyncReadOffset(emitterID);
			}
		}
	}

	void ParticleManager::ResetMetrics()
	{
		m_rebuildCount = 0;
		m_avgRebuildTime = 0.0f;
		m_totalRebuildTime = 0.0f;
	}
}