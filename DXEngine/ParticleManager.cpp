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
		// Page Table 20% 이상 단편화되면 재구성
		constexpr float DEFRAG_THRESHOLD = 0.2f;
		if (m_memoryPool->GetFragmentationRatio() >= DEFRAG_THRESHOLD) {
			//  재구성 시간 측정
			auto start = std::chrono::high_resolution_clock::now();
			
			m_memoryPool->RebuildPageTable(m_activeSystems);
			
			for (auto* system : m_activeSystems) {
				if (system) {
					UploadEmitterIDs(system, system->GetInitialData());
				}
			}
			
			auto end = std::chrono::high_resolution_clock::now();
			float elapsed = std::chrono::duration<float, std::milli>(end - start).count();
			
			m_rebuildCount++;
			m_totalRebuildTime += elapsed;
			m_avgRebuildTime = m_totalRebuildTime / m_rebuildCount;
		}

		m_memoryPool->ClearWriteCount();
		m_memoryPool->BindCompute();

		for (auto* system : m_activeSystems) {
			if (system) {
				std::vector<ParticleFrameConsts> fsConsts(system->GetMaxEmitterCount());
				system->PreUpdate(dt, fsConsts);
				m_memoryPool->UploadFrameConsts(system->GetPoolHandle().emitterIDs, fsConsts);
			}
		}

		m_memoryPool->UpdateArgs();

		for (auto* system : m_activeSystems) {
			if (system) {
				m_memoryPool->BindMeshConsts(system->GetPoolHandle().systemSlot);
				system->Update(dt);
			}
		}

		m_memoryPool->UnbindCompute();

		if (m_memoryPool)
			m_memoryPool->SwapBuffer();
	}

	void ParticleManager::Render()
	{
		if (m_activeSystems.empty()) return;

		m_memoryPool->BindRender();
		for (auto* system : m_activeSystems) {
			if (system) {
				m_memoryPool->BindMeshConsts(system->GetPoolHandle().systemSlot);
				system->Render();
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
		// 1. Prototype 로드 또는 캐시에서 가져오기
		auto prototypeIt = m_prototypes.find(path);
		ParticleSystem* prototype = nullptr;

		if (prototypeIt != m_prototypes.end()) {
			prototype = prototypeIt->second.get();
		}
		else {
			// Prototype 최초 로드 시에만 InitializeCPU 실행
			auto newSystem = ParticleLoader::Load<ParticleSystem>(path);
			if (!newSystem) return nullptr;
			
			ParticleInitializer initialData;
			newSystem->InitializeCPU(initialData, m_memoryPool->GetBlockSize());  // InitializeCPU 포함
			
			prototype = newSystem.get();
			m_prototypes[path] = std::move(newSystem);
		}

		// 2. Clone 생성 (복사 생성자만 - InitializeCPU 생략)
		auto cloned = std::make_unique<ParticleSystem>(*prototype);
		
		// 3. Prototype의 캐싱된 initialData 재사용
		const ParticleInitializer& initialData = prototype->GetInitialData();

		// 4. 메모리 할당
		PoolHandle handle = m_memoryPool->Allocate(
			prototype->GetTotalBlockCount(),
			prototype->GetMaxEmitterCount(),
			static_cast<UINT>(initialData.spawnPositions.size())
		);
		
		if (!handle.IsActive()) return nullptr;
		
		cloned->SetPoolHandle(handle);

		// 5. PageTable 추가
		UINT pageTableOffset = m_memoryPool->AppendToPageTable(handle.particleIndices);
		cloned->SetPageTableOffset(pageTableOffset);

		// 6. GPU 업로드 (Prototype의 데이터 재사용)
		if (!initialData.spawnPositions.empty() && handle.spawnPosOffset != UINT_MAX) {
			m_memoryPool->UploadSpawnPositions(handle.spawnPosOffset, initialData.spawnPositions);
		}

		cloned->InitializeGPU(
			m_memoryPool->GetDispatchArgs(),
			m_memoryPool->GetBillboardArgs(),
			m_memoryPool->GetMeshArgs());

		m_memoryPool->UploadConsts(handle.emitterIDs, initialData.consts);
		m_memoryPool->UploadFrameConsts(handle.emitterIDs, initialData.frameConsts);

		UploadEmitterIDs(cloned.get(), initialData);
		cloned->OnSpawn();

		ParticleSystem* rawPtr = cloned.get();
		m_instances.push_back(std::move(cloned));
		RegisterActiveSystem(rawPtr);

		return rawPtr;
	}
	
	void ParticleManager::DestroyInstance(ParticleSystem* system)
	{
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
				float usage = (totalBlocks > 0) ? static_cast<float>(usedBlocks) / totalBlocks : 0.0f;

				ImGui::Text("Particle Blocks: %d / %d (Size: %d)", usedBlocks, totalBlocks, m_memoryPool->GetBlockSize());
				ImGui::ProgressBar(usage, ImVec2(-1, 0), "Particle Memory Usage");
				ImGui::Text("Max Total Particles Count : %d", usedBlocks * m_memoryPool->GetBlockSize());

				UINT totalEmitters = m_memoryPool->GetTotalEmitterSlots();
				UINT usedEmitters = m_memoryPool->GetUsedEmitterSlots();
				float emitterUsage = (totalEmitters > 0) ? static_cast<float>(usedEmitters) / totalEmitters : 0.0f;

				ImGui::Text("Emitter Slots: %d / %d", usedEmitters, totalEmitters);
				ImGui::ProgressBar(emitterUsage, ImVec2(-1, 0), "Emitter Slot Usage");

				UINT totalSystems = m_memoryPool->GetTotalSystemSlots();
				UINT usedSystems = m_memoryPool->GetUsedSystemSlots();
				ImGui::Text("Active Systems: %d / %d", usedSystems, totalSystems);
				
				// Page Table 단편화율 표시
				float fragRatio = m_memoryPool->GetFragmentationRatio();
				ImVec4 fragColor = (fragRatio < 0.2f) ? ImVec4(0, 1, 0, 1) :
					(fragRatio < 0.5f) ? ImVec4(1, 1, 0, 1) : ImVec4(1, 0, 0, 1);
				ImGui::TextColored(fragColor, "Fragmentation: %.1f%%", fragRatio * 100.0f);

				// Page Table 크기 (Paging 방식)
				ImGui::Text("Page Table Used: %d entries", m_memoryPool->GetPageTableUsedSize());
			}
			//  성능 메트릭 섹션 추가
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
			if (ImGui::CollapsingHeader("Block Map (Visualizer)", ImGuiTreeNodeFlags_DefaultOpen))
			{
				UINT totalBlocks = m_memoryPool->GetTotalBlockCount();
				std::vector<std::string> blockOwners(totalBlocks, "Free");

				for (auto* system : m_activeSystems) {
					if (!system) continue;
					const auto& indices = system->GetPoolHandle().particleIndices;
					std::string name = std::string(system->GetName().begin(), system->GetName().end());

					for (UINT blockIdx : indices) {
						if (blockIdx < totalBlocks) {
							blockOwners[blockIdx] = name;
						}
					}
				}

				const auto& table = m_memoryPool->GetParticleBlockTable();
				int columns = 32;
				float cellSize = 10.0f;
				float spacing = 2.0f;

				ImVec2 p = ImGui::GetCursorScreenPos();
				float startX = p.x;
				float startY = p.y;

				for (size_t i = 0; i < table.size(); ++i)
				{
					float x = startX + (i % columns) * (cellSize + spacing);
					float y = startY + (i / columns) * (cellSize + spacing);

					ImU32 color = table[i] ? IM_COL32(50, 205, 50, 255) : IM_COL32(50, 50, 50, 255);

					ImGui::GetWindowDrawList()->AddRectFilled(
						ImVec2(x, y),
						ImVec2(x + cellSize, y + cellSize),
						color
					);

					if (ImGui::IsMouseHoveringRect(ImVec2(x, y), ImVec2(x + cellSize, y + cellSize)))
					{
						ImGui::BeginTooltip();
						ImGui::Text("Block ID: %llu", i);
						ImGui::TextColored(table[i] ? ImVec4(0, 1, 0, 1) : ImVec4(0.5, 0.5, 0.5, 1),
							"Status: %s", table[i] ? "Used" : "Free");

						if (table[i]) {
							ImGui::Text("Owner: %s", blockOwners[i].c_str());
						}
						ImGui::EndTooltip();
					}
				}

				float totalHeight = ((table.size() + columns - 1) / columns) * (cellSize + spacing);
				ImGui::Dummy(ImVec2(0, totalHeight));
			}
		}
		ImGui::End();
	}

	void ParticleManager::UploadEmitterIDs(ParticleSystem* system, const ParticleInitializer& initialData)
	{
		const PoolHandle& handle = system->GetPoolHandle();
		
		UINT blockCount = 0;
		for (size_t i = 0; i < initialData.emitterIDs.size(); ++i) {
			EmitterID eID = initialData.emitterIDs[i];
			
			eID.emitterID = handle.emitterIDs[i];
			eID.pageTableOffset = system->GetPageTableOffset() + blockCount;
			blockCount += eID.blockCount;
			
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
		
		m_memoryPool->UploadMeshConsts(systemSlot, pmConsts);
	}

	void ParticleManager::BindMeshConsts(UINT systemSlot)
	{
		m_memoryPool->BindMeshConsts(systemSlot);
	}
}