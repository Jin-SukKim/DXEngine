#include "pch.h"
#include "ParticleManager.h"
#include "ParticleLoader.h"
#include "RenderBase.h"

namespace DE {
	void ParticleManager::Initialize()
	{
		m_memoryPool = std::make_unique<ParticleMemoryPool>();
		m_memoryPool->Initialize(10000000, 10000, 10000);
	}

	void ParticleManager::Update(const float& dt)
	{
		// 1. Update 전체 통합 시간 측정
		ScopedTimer timer([&](float t) { UpdateMetric(m_runtimeProfile.update, t); });

		// Defragment는 별도 함수 내부에서 측정 중
		constexpr float DEFRAG_THRESHOLD = 0.2f;
		if (m_memoryPool->GetFragmentationRatio() >= DEFRAG_THRESHOLD) {
			Defragment();
		}

		// 2. [세부 측정] 데이터 준비 및 업로드
		{
			ScopedTimer tPrepare([&](float t) { UpdateMetric(m_runtimeProfile.update_prepare, t); });

			// (A) Setup: Compute 쉐이더 바인딩 및 초기화
			{
				ScopedTimer tSetup([&](float t) { UpdateMetric(m_runtimeProfile.update_prepare_setup, t); });
				m_memoryPool->ClearWriteCount();
				m_memoryPool->BindCompute();
			}

			// (B) CPU: 데이터 갱신 루프 (PreUpdate)
			{
				ScopedTimer tCpu([&](float t) { UpdateMetric(m_runtimeProfile.update_prepare_cpu, t); });

				for (auto* system : m_activeSystems) {
					if (system) {
						// 주의: 루프 내부 할당이 성능에 영향을 줄 수 있음
						std::vector<ParticleFrameConsts> fsConsts(system->GetMaxEmitterCount());
						system->PreUpdate(dt, m_memoryPool->GetFrameConsts().GetCpu());
					}
				}
			}

			// ★ 모든 버퍼를 한번에 업로드
			{
				ScopedTimer tUpload([&](float t) { UpdateMetric(m_runtimeProfile.update_prepare_upload, t); });
				m_memoryPool->UploadFrameConsts();
				m_memoryPool->UploadEmitterIDs();
				m_memoryPool->UploadAllMeshConsts();
			}
		}

		// 3. Indirect Args Update
		{
			ScopedTimer tArgs([&](float t) { UpdateMetric(m_runtimeProfile.update_args, t); });
			m_memoryPool->UpdateArgs();
		}

		// 4. System Logic Dispatch - ★ 개별 BindMeshConsts 제거
		{
			ScopedTimer tDispatch([&](float t) { UpdateMetric(m_runtimeProfile.update_dispatch, t); });

			for (auto* system : m_activeSystems) {
				if (system) {
					// ★ BindMeshConsts 호출 제거 - Shader에서 SRV로 직접 인덱싱
					system->Update(dt);
				}
			}
			m_memoryPool->UnbindCompute();
		}

		// SyncReadOffsets는 내부에서 측정 중
		if (m_needsSyncReadOffset) {
			SyncReadOffsets();
			m_needsSyncReadOffset = false;
		}

		// 5. Swap Buffer
		if (m_memoryPool)
		{
			ScopedTimer tSwap([&](float t) { UpdateMetric(m_runtimeProfile.update_swap, t); });
			m_memoryPool->SwapBuffer();
		}
	}

	void ParticleManager::Render()
	{
		// ★ Render 시간 측정
		ScopedTimer timer([&](float t) { UpdateMetric(m_runtimeProfile.render, t); });

		if (!m_activeSystems.empty()) {
			m_memoryPool->BindRender();
			for (auto* system : m_activeSystems) {
				if (system) {
					// ★ BindMeshConsts 호출 제거 - Shader에서 SRV로 직접 인덱싱
					system->Render();
				}
			}
			m_memoryPool->UnbindRender();
		}
		GET_SINGLE(RenderBase)->SetPipelineState(RenderBase::graphicsCommon.basic.solidPSO);
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

		// 1. Prototype Load (Cache 체크)
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
		m_memoryPool->UploadFrameConsts(handle.emitterIDs, initialData.frameConsts);
		
		// ★ 생성 시점에 즉시 업로드
		m_memoryPool->UploadEmitterIDs();

		clonedPtr->Initialize(initialData);
		clonedPtr->OnSpawn();

		m_instances.push_back(std::move(cloned));
		RegisterActiveSystem(clonedPtr);

		return clonedPtr;
	}
	
	void ParticleManager::DestroyInstance(ParticleSystem* system)
	{
		// ★ Destroy 시간 측정
		ScopedTimer timer([&](float t) { UpdateMetric(m_runtimeProfile.destroy, t); });

		if (!system) return;
		const PoolHandle& handle = system->GetPoolHandle();
		if (handle.IsActive()) { m_memoryPool->Free(handle); }
		UnregisterActiveSystem(system);

		auto it = std::find_if(m_instances.begin(), m_instances.end(),
			[system](const std::unique_ptr<ParticleSystem>& ptr) { return ptr.get() == system; });

		if (it != m_instances.end()) { m_instances.erase(it); }
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
				
				// ★ 단편화 비율
				float fragRatio = m_memoryPool->GetFragmentationRatio();
				ImVec4 fragColor = (fragRatio < 0.2f) ? ImVec4(0,1,0,1) : 
								(fragRatio < 0.5f) ? ImVec4(1,1,0,1) : ImVec4(1,0,0,1);
				ImGui::TextColored(fragColor, "Fragmentation: %.1f%%", fragRatio * 100.0f);
			}
			
			// ★ 성능 메트릭 섹션 추가
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
			
			// 2. 블록 시각화 (Grid Visualizer)
			// 녹색: 사용 중, 회색: 빈 공간
			if (ImGui::CollapsingHeader("Block Map (Visualizer)", ImGuiTreeNodeFlags_DefaultOpen))
			{
				UINT totalBlocks = m_memoryPool->GetTotalBlockCount();
				std::vector<std::string> blockOwners(totalBlocks, "Free");

				const auto& table = m_memoryPool->GetParticleBlockTable();
				int columns = 32; // 한 줄에 보여줄 블록 개수
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

					// 마우스 오버 시 블록 번호 툴팁
					if (ImGui::IsMouseHoveringRect(ImVec2(x, y), ImVec2(x + cellSize, y + cellSize)))
					{
						ImGui::BeginTooltip();
						// [변경] 소유자 이름까지 출력
						ImGui::Text("Block ID: %llu", i);
						ImGui::TextColored(table[i] ? ImVec4(0, 1, 0, 1) : ImVec4(0.5, 0.5, 0.5, 1),
							"Status: %s", table[i] ? "Used" : "Free");

						if (table[i]) {
							ImGui::Text("Owner: %s", blockOwners[i].c_str());
						}
						ImGui::EndTooltip();
					}
				}

				// 그리드 높이만큼 커서 이동
				float totalHeight = ((table.size() + columns - 1) / columns) * (cellSize + spacing);
				ImGui::Dummy(ImVec2(0, totalHeight));
			}
		}
		ImGui::End();
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

					// [변경] Update 상세 내역 계층화
					AddRow("Update (Total)", m_runtimeProfile.update, ImVec4(1.0f, 0.6f, 0.0f, 1.0f));

					ImVec4 subColor = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
					AddRow("  - Prepare & Upload (Total)", m_runtimeProfile.update_prepare, subColor);

					// ★ Prepare 세부 항목 (들여쓰기)
					ImVec4 detailColor = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
					AddRow("      > Setup (Bind)", m_runtimeProfile.update_prepare_setup, detailColor);
					AddRow("      > CPU PreUpdate", m_runtimeProfile.update_prepare_cpu, detailColor);
					AddRow("      > GPU Upload", m_runtimeProfile.update_prepare_upload, detailColor);

					AddRow("  - Update Args", m_runtimeProfile.update_args, subColor);
					AddRow("  - Dispatch Logic", m_runtimeProfile.update_dispatch, subColor);
					AddRow("  - Swap Buffer", m_runtimeProfile.update_swap, subColor);

					ImGui::TableNextRow(); // 공백

					AddRow("Render (Total)", m_runtimeProfile.render, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
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
		// ★ 측정
		ScopedTimer timer([&](float t) { UpdateMetric(m_runtimeProfile.requestAlloc, t); });

		PoolHandle handle = m_memoryPool->Allocate(particleCount, emitterCount, spawnPosCount);
		if (!handle.IsActive()) { m_needsDefragment = true; }
		return handle;
	}

	void ParticleManager::UploadEmitterIDs(ParticleSystem* system, const ParticleInitializer& initialData)
	{
		// ★ 측정
		ScopedTimer timer([&](float t) { UpdateMetric(m_runtimeProfile.uploadIDs, t); });

		const PoolHandle& handle = system->GetPoolHandle();

		for (size_t i = 0; i < initialData.emitterIDs.size(); ++i) {
			EmitterID eID = initialData.emitterIDs[i];
			eID.emitterID = handle.emitterIDs[i];
			eID.readParticleOffset = handle.particleOffset + initialData.emitterIDs[i].readParticleOffset;
			eID.writeParticleOffset = handle.particleOffset + initialData.emitterIDs[i].writeParticleOffset;
			// ★ systemSlot 추가
			eID.systemSlot = handle.systemSlot;
			if (handle.spawnPosOffset != UINT_MAX && eID.spawnPosOffset != UINT_MAX) {
				eID.spawnPosOffset += handle.spawnPosOffset;
			}
			m_memoryPool->UpdateEmitterID(handle.emitterIDs[i], eID);
		}
	}

	void ParticleManager::UploadMeshConsts(UINT systemSlot, const MeshConstants& data)
	{
		ParticleMeshConsts pmConsts;
		pmConsts.world = data.world;
		pmConsts.worldIT = data.worldIT;
		pmConsts.vertexCount = 0;
		pmConsts.indexCount = 0;
		pmConsts.systemSlot = systemSlot;
		
		m_memoryPool->UploadMeshConsts(systemSlot, pmConsts);
	}

	void ParticleManager::Defragment()
	{
		// ★ 측정
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

		m_needsSyncReadOffset = true;
	}

	void ParticleManager::RecalculateEmitterOffsets(ParticleSystem* system, UINT newParticleOffset)
	{
		// ★ 측정
		ScopedTimer timer([&](float t) { UpdateMetric(m_runtimeProfile.recalculateOffsets, t); });

		PoolHandle& handle = system->GetPoolHandle();
		const ParticleInitializer& initialData = system->GetInitialData();

		for (size_t i = 0; i < handle.emitterIDs.size(); ++i) {
			UINT globalEmitterID = handle.emitterIDs[i];
			UINT localOffset = initialData.emitterIDs[i].writeParticleOffset;
			m_memoryPool->UpdateWriteOffset(globalEmitterID, newParticleOffset + localOffset);
		}
		handle.particleOffset = newParticleOffset;
	}

	void ParticleManager::SyncReadOffsets()
	{
		// ★ 측정
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