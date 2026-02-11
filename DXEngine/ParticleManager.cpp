#include "pch.h"
#include "ParticleManager.h"
#include "ParticleLoader.h"
#include "RenderBase.h"
#include "ScopedTimer.h" // [Added] Include ScopedTimer
#include "ModelManager.h"
#include "TextureManager.h"
#include <DirectXCollision.h> // [Added] For Frustum Culling

namespace DE {
	void ParticleManager::Initialize()
	{
		m_memoryPool = std::make_unique<ParticleMemoryPool>();
		m_memoryPool->Initialize(10000000, 20000, 20000);
		TextureManager::Get().BindParticleTextures();
	}

	void ParticleManager::Update(const float& dt)
	{
		// Track time for priority age calculation
		m_currentTime += dt;

		// [Added] Measure Total Update Time
		ScopedTimer timer([&](float t) { UpdateMetric(m_runtimeProfile.update, t); });

		constexpr float DEFRAG_THRESHOLD = 0.2f;
		if (m_memoryPool->GetFragmentationRatio() >= DEFRAG_THRESHOLD) {
			//  籸 ð  (Legacy Metric - Cumulative Average)
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
				// Mesh와 Billboard 모두 PreUpdate
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
				m_memoryPool->UploadMeshConsts();
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
			// Mesh와 Billboard 모두 Update
			for (auto* system : m_activeSystems) {
				if (system) {
					system->Update(dt);
				}
			}
			m_memoryPool->UnbindCompute();
		}

		// Compute , SwapBuffer  Read  ȭ
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
		// [Added] Measure Render Time
		ScopedTimer timer([&](float t) { UpdateMetric(m_runtimeProfile.render, t); });

		if (m_activeSystems.empty()) return;

		// [Added] CPU Frustum Culling
		// Projection 행렬로 view space frustum 생성
		DirectX::BoundingFrustum frustum(m_proj);

		// [Added] Reset Culling Stats
		UINT totalCount = 0;
		UINT visibleCount = 0;

		// Extract camera position from view matrix (inverse translation)
		Matrix invView = m_view.Invert();
		Vector3 cameraPos(invView._41, invView._42, invView._43);

		// Update spawn ratios for overdraw control before rendering
		for (auto* system : m_activeSystems) {
			if (system) {
				system->UpdateSpawnRatios(cameraPos);
			}
		}

		m_memoryPool->UploadFrameConsts();
		m_memoryPool->BindRender();
		m_memoryPool->UpdateRenderArgs();

		std::vector< ParticleSystem*> renderSystems;
		for (auto* system : m_activeSystems) {
			if (system) {
				totalCount++;
				// Frustum Culling Test (view space)
				Vector3 posWorld = system->GetWorldPosition();
				Vector3 posView = Vector3::Transform(posWorld, m_view);  // World -> View space
				float radius = system->GetBoundingRadius();
				DirectX::BoundingSphere sphere(posView, radius);

				if (frustum.Intersects(sphere)) {
					visibleCount++;
					renderSystems.push_back(system);
				}
			}
		}

		auto context = GET_SINGLE(RenderBase)->GetContext();
		ModelManager::Get().BindBuffersForRender();
		// 1. Mesh RenderModule 먼저 렌더링 (depth buffer 채우기)
		GET_SINGLE(RenderBase)->SetPipelineState(RenderBase::graphicsCommon.particle.meshPSO);
		for (auto* system : renderSystems) {
			if (system) {
				system->RenderMesh();
			}
		}

		// 2. Billboard RenderModule 나중에 렌더링 (overdraw 감소)
		//GET_SINGLE(RenderBase)->SetLowResRender();
		context->OMSetBlendState(RenderBase::graphicsCommon.accumulateBS.Get(), RenderBase::graphicsCommon.particle.animPSO.blendFactor, 0xffffffff);
		GET_SINGLE(RenderBase)->SetPipelineState(RenderBase::graphicsCommon.particle.billboardInstancedPSO);
		
		for (auto* system : renderSystems) {
			if (system) {
				system->RenderBillboard();
			}
		}

		m_memoryPool->UnbindRender();
		//GET_SINGLE(RenderBase)->RenderCompositeLowResParticles();

		// [Added] Update Culling Stats
		m_runtimeProfile.totalSystems = totalCount;
		m_runtimeProfile.visibleSystems = visibleCount;
		m_runtimeProfile.culledSystems = totalCount - visibleCount;
	}

	void ParticleManager::RenderDepth()
	{
		if (m_activeSystems.empty()) return;

		// [Added] CPU Frustum Culling
		// Projection 행렬로 view space frustum 생성
		DirectX::BoundingFrustum frustum(m_proj);

		// Extract camera position from view matrix (inverse translation)
		Matrix invView = m_view.Invert();
		Vector3 cameraPos(invView._41, invView._42, invView._43);

		// Update spawn ratios for overdraw control before rendering
		for (auto* system : m_activeSystems) {
			if (system) {
				system->UpdateSpawnRatios(cameraPos);
			}
		}
		for (auto* system : m_activeSystems) {
			if (system) {
				system->UpdateSpawnRatios(cameraPos);
			}
		}

		m_memoryPool->UploadFrameConsts();
		m_memoryPool->BindRender();
		m_memoryPool->UpdateRenderArgs();
		ModelManager::Get().BindBuffersForRender();
		// 1. Mesh RenderModule 먼저 렌더링 (depth buffer 채우기)
		for (auto* system : m_activeSystems) {
			if (system) {
				// Frustum Culling Test (view space)
				Vector3 posWorld = system->GetWorldPosition();
				Vector3 posView = Vector3::Transform(posWorld, m_view);  // World -> View space
				float radius = system->GetBoundingRadius();
				DirectX::BoundingSphere sphere(posView, radius);

				if (frustum.Intersects(sphere)) {
					system->RenderMesh();
				}
			}
		}

		m_memoryPool->UnbindRender();
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

		// Try eviction if allocation failed
		if (!handle.IsActive())
		{
			if (!TryEvictAndRetry(particleCount, emitterCount, spawnPosCount, handle))
			{
				// Still failed after eviction attempts
				return nullptr;
			}
		}

		// Set creation timestamp
		clonedPtr->SetCreationTime(m_currentTime);

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

				//  ȭ 
				float fragRatio = m_memoryPool->GetFragmentationRatio();
				ImVec4 fragColor = (fragRatio < 0.2f) ? ImVec4(0, 1, 0, 1) :
					(fragRatio < 0.5f) ? ImVec4(1, 1, 0, 1) : ImVec4(1, 0, 0, 1);
				ImGui::TextColored(fragColor, "Fragmentation: %.1f%%", fragRatio * 100.0f);
			}

			//   Ʈ  ߰
			if (ImGui::CollapsingHeader("Performance Metrics", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Text("Rebuild Count: %d", m_rebuildCount);
				ImGui::Text("Avg Rebuild Time: %.3f ms", m_avgRebuildTime);
				ImGui::Text("Total Rebuild Time: %.3f ms", m_totalRebuildTime);

#ifdef _DEBUG
				ImGui::Separator();
				ImGui::Text("Memory Pool Allocations:");
				ImGui::Text("  Allocate Calls: %u", m_memoryPool->GetAllocateCallCount());
				ImGui::Text("  Avg Allocate Time: %.6f ms", m_memoryPool->GetAvgAllocateTime() / 1000.0);
				ImGui::Text("  Free Calls: %u", m_memoryPool->GetFreeCallCount());
				ImGui::Text("  Avg Free Time: %.6f ms", m_memoryPool->GetAvgFreeTime() / 1000.0);
#endif

				if (ImGui::Button("Reset Metrics")) {
					m_rebuildCount = 0;
					m_avgRebuildTime = 0.0f;
					m_totalRebuildTime = 0.0f;
				}
			}

			// 2.  ðȭ (Grid Visualizer)
			// :  , ȸ:  
			if (ImGui::CollapsingHeader("Block Map (Visualizer)", ImGuiTreeNodeFlags_DefaultOpen))
			{
				UINT totalBlocks = m_memoryPool->GetTotalBlockCount();
				std::vector<std::string> blockOwners(totalBlocks, "Free");

				const auto& table = m_memoryPool->GetParticleBlockTable();
				int columns = 32; //  ٿ   
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

					// 콺    ȣ 
					if (ImGui::IsMouseHoveringRect(ImVec2(x, y), ImVec2(x + cellSize, y + cellSize)))
					{
						ImGui::BeginTooltip();
						// []  ̸ 
						ImGui::Text("Block ID: %llu", i);
						ImGui::TextColored(table[i] ? ImVec4(0, 1, 0, 1) : ImVec4(0.5, 0.5, 0.5, 1),
							"Status: %s", table[i] ? "Used" : "Free");

						if (table[i]) {
							ImGui::Text("Owner: %s", blockOwners[i].c_str());
						}
						ImGui::EndTooltip();
					}
				}

				// ׸ ̸ŭ Ŀ ̵
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
					AddRow("DestroyInstance", m_runtimeProfile.destroy);
					AddRow("Defragment (Check+Run)", m_runtimeProfile.defrag, (m_runtimeProfile.defrag > 0.1f) ? ImVec4(1, 0, 0, 1) : ImVec4(1, 1, 1, 1));

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::TextDisabled("--- Internal Helpers ---");

					AddRow("  RequestAllocation", m_runtimeProfile.requestAlloc);
					AddRow("  UploadEmitterIDs", m_runtimeProfile.uploadIDs);
					AddRow("  RecalculateOffsets", m_runtimeProfile.recalculateOffsets);
					AddRow("  SyncReadOffsets", m_runtimeProfile.syncReadOffsets);
					AddRow("Eviction Logic", m_runtimeProfile.eviction,
						(m_runtimeProfile.eviction > 0.1f) ? ImVec4(1, 0, 0, 1) : ImVec4(1, 1, 1, 1));

					ImGui::EndTable();
				}

				if (ImGui::Button("Reset All Runtime Metrics", ImVec2(-1, 0))) {
					m_runtimeProfile.Reset();
				}
			}

			// [Added] Frustum Culling Statistics
			if (ImGui::CollapsingHeader("Frustum Culling Stats", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Text("Total Particle Systems: %u", m_runtimeProfile.totalSystems);
				ImGui::Text("Visible Systems: %u", m_runtimeProfile.visibleSystems);
				ImGui::Text("Culled Systems: %u", m_runtimeProfile.culledSystems);

				if (m_runtimeProfile.totalSystems > 0) {
					float cullRatio = (float)m_runtimeProfile.culledSystems / m_runtimeProfile.totalSystems * 100.0f;
					ImVec4 cullColor = (cullRatio < 30.0f) ? ImVec4(1, 0, 0, 1) :
						(cullRatio < 60.0f) ? ImVec4(1, 1, 0, 1) : ImVec4(0, 1, 0, 1);
					ImGui::TextColored(cullColor, "Cull Ratio: %.1f%%", cullRatio);
				}

				// Debug: Show first few systems' positions
				ImGui::Separator();
				ImGui::Text("Debug - Active Systems:");
				for (int i = 0; i < 5 && i < m_activeSystems.size(); ++i) {
					auto sys = m_activeSystems[i];
					if (sys) {
						Vector3 pos = sys->GetWorldPosition();
						ImGui::Text("  Sys[%d]: Pos(%.1f, %.1f, %.1f) R:%.1f",
							i, pos.x, pos.y, pos.z, sys->GetBoundingRadius());
					}
				}

				// Debug: View/Proj matrices
				ImGui::Separator();
				if (ImGui::TreeNode("View Matrix (Debug)")) {
					ImGui::Text("Row 0: %.2f, %.2f, %.2f, %.2f", m_view._11, m_view._12, m_view._13, m_view._14);
					ImGui::Text("Row 1: %.2f, %.2f, %.2f, %.2f", m_view._21, m_view._22, m_view._23, m_view._24);
					ImGui::Text("Row 2: %.2f, %.2f, %.2f, %.2f", m_view._31, m_view._32, m_view._33, m_view._34);
					ImGui::Text("Row 3: %.2f, %.2f, %.2f, %.2f", m_view._41, m_view._42, m_view._43, m_view._44);
					ImGui::TreePop();
				}
				if (ImGui::TreeNode("Proj Matrix (Debug)")) {
					ImGui::Text("Row 0: %.2f, %.2f, %.2f, %.2f", m_proj._11, m_proj._12, m_proj._13, m_proj._14);
					ImGui::Text("Row 1: %.2f, %.2f, %.2f, %.2f", m_proj._21, m_proj._22, m_proj._23, m_proj._24);
					ImGui::Text("Row 2: %.2f, %.2f, %.2f, %.2f", m_proj._31, m_proj._32, m_proj._33, m_proj._34);
					ImGui::Text("Row 3: %.2f, %.2f, %.2f, %.2f", m_proj._41, m_proj._42, m_proj._43, m_proj._44);
					ImGui::TreePop();
				}
			}

			// [Added] Priority-Based Eviction Stats
			if (ImGui::CollapsingHeader("Priority-Based Eviction", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Text("Total Evictions: %u", m_runtimeProfile.evictionCount);
				ImGui::Text("Eviction Failures: %u", m_runtimeProfile.evictionFailures);
				ImGui::Text("Avg Eviction Time: %.4f ms", m_runtimeProfile.eviction);

				ImGui::Separator();
				ImGui::Text("Current System Priorities:");

				if (!m_instances.empty()) {
					Matrix invView = m_view.Invert();
					Vector3 cameraPos(invView._41, invView._42, invView._43);
					DirectX::BoundingFrustum frustum(m_proj);

					for (const auto& instance : m_instances)
					{
						ParticleSystem* sys = instance.get();
						if (!sys) continue;

						float priority = CalculatePriority(sys, cameraPos, frustum);
						float age = m_currentTime - sys->GetCreationTime();

						std::string name = std::string(sys->GetName().begin(), sys->GetName().end());
						ImGui::Text("  [%s] Priority: %.3f | Age: %.1fs | Base: %.2f",
							name.c_str(), priority, age, sys->GetBasePriority());
					}
				}
				else {
					ImGui::TextDisabled("  No active systems");
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

		// Defragment  
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
			eID.systemID = handle.systemSlot;
			eID.readParticleOffset = handle.particleOffset + initialData.emitterIDs[i].readParticleOffset;
			eID.writeParticleOffset = handle.particleOffset + initialData.emitterIDs[i].writeParticleOffset;

			// spawnPos ϴ emitter  
			if (handle.spawnPosOffset != UINT_MAX && eID.spawnPosOffset != UINT_MAX) {
				eID.spawnPosOffset += handle.spawnPosOffset;
			}

			m_memoryPool->UpdateEmitterID(handle.emitterIDs[i], eID);
		}
	}

	void ParticleManager::UpdateMeshConsts(UINT systemSlot, const ParticleMeshConsts& data)
	{
		m_memoryPool->UpdateMeshConsts(systemSlot, data);
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

		m_needsSyncReadOffset = true; 
	}

	void ParticleManager::RecalculateEmitterOffsets(ParticleSystem* system, UINT newParticleOffset)
	{
		// [Added] Measure Offset Recalculation Time
		ScopedTimer timer([&](float t) { UpdateMetric(m_runtimeProfile.recalculateOffsets, t); });

		PoolHandle& handle = system->GetPoolHandle();
		const ParticleInitializer& initialData = system->GetInitialData();

		for (size_t i = 0; i < handle.emitterIDs.size(); ++i) {
			// writeParticleOffset  ġ 
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

		// 두 배열 모두 순회
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

	float ParticleManager::CalculatePriority(
		ParticleSystem* system,
		const Vector3& cameraPos,
		const DirectX::BoundingFrustum& frustum) const
	{
		// 1. Distance Factor (closer = higher priority)
		Vector3 systemPos = system->GetWorldPosition();
		float distance = Vector3::Distance(systemPos, cameraPos);

		// Normalize to [0, 1] with inverse square falloff
		constexpr float MAX_DISTANCE = 100.0f;
		float distanceFactor = 1.0f - std::min(distance / MAX_DISTANCE, 1.0f);
		distanceFactor = distanceFactor * distanceFactor;  // Square for sharper falloff

		// 2. Visibility Factor (inside frustum = 1.0, outside = 0.0)
		Vector3 posView = Vector3::Transform(systemPos, m_view);
		float radius = system->GetBoundingRadius();
		DirectX::BoundingSphere sphere(posView, radius);
		float visibilityFactor = frustum.Intersects(sphere) ? 1.0f : 0.0f;

		// 3. Age Factor (newer = slightly higher priority)
		float age = m_currentTime - system->GetCreationTime();
		constexpr float AGE_THRESHOLD = 5.0f;  // Systems < 5s old get bonus
		float ageFactor = std::max(0.0f, 1.0f - (age / AGE_THRESHOLD));

		// 4. Base Priority Factor (user-defined)
		float basePriorityFactor = system->GetBasePriority();

		// Weighted sum: distance(40%) + visibility(30%) + age(20%) + base(10%)
		float priority = (distanceFactor * 0.4f) +
			(visibilityFactor * 0.3f) +
			(ageFactor * 0.2f) +
			(basePriorityFactor * 0.1f);

		return priority;
	}

	ParticleSystem* ParticleManager::FindLowestPrioritySystem(UINT particleCount)
	{
		if (m_instances.empty())
			return nullptr;

		// Extract camera position from view matrix
		Matrix invView = m_view.Invert();
		Vector3 cameraPos(invView._41, invView._42, invView._43);

		// Create frustum for visibility check
		DirectX::BoundingFrustum frustum(m_proj);

		// Linear search for lowest priority
		ParticleSystem* lowestPrioritySystem = nullptr;
		float lowestPriority = FLT_MAX;

		for (const auto& instance : m_instances)
		{
			ParticleSystem* system = instance.get();
			if (!system) continue;

			float priority = CalculatePriority(system, cameraPos, frustum);

			if (priority < lowestPriority)
			{
				lowestPriority = priority;
				lowestPrioritySystem = system;
			}
		}

		return lowestPrioritySystem;
	}

	bool ParticleManager::TryEvictAndRetry(
		UINT particleCount,
		UINT emitterCount,
		UINT spawnPosCount,
		PoolHandle& outHandle)
	{
		ScopedTimer timer([&](float t) { UpdateMetric(m_runtimeProfile.eviction, t); });

		constexpr int MAX_EVICTION_ATTEMPTS = 3;

		for (int attempt = 0; attempt < MAX_EVICTION_ATTEMPTS; ++attempt)
		{
			// Find lowest priority system
			ParticleSystem* victim = FindLowestPrioritySystem(particleCount);

			if (!victim)
			{
				// No systems available to evict
				m_runtimeProfile.evictionFailures++;
				return false;
			}

			// Evict the victim
			DestroyInstance(victim);
			m_runtimeProfile.evictionCount++;

			// Retry allocation
			outHandle = RequestAllocation(particleCount, emitterCount, spawnPosCount);

			if (outHandle.IsActive())
			{
				// Success!
				return true;
			}
		}

		// Failed after max attempts
		m_runtimeProfile.evictionFailures++;
		return false;
	}
}