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
		auto prototypeIt = m_prototypes.find(path);
		ParticleSystem* prototype = nullptr;

		if (prototypeIt != m_prototypes.end()) {
			prototype = prototypeIt->second.get();
		}
		else {
			auto newSystem = ParticleLoader::Load<ParticleSystem>(path);
			if (!newSystem) {
				return nullptr;
			}
			newSystem->Initialize();
			prototype = newSystem.get();
			m_prototypes[path] = std::move(newSystem);
		}

		auto cloned = std::make_unique<ParticleSystem>(*prototype);

		ParticleInitializer initialData;
		cloned->InitializeCPU(initialData);

		UINT spawnPosCount = static_cast<UINT>(initialData.spawnPositions.size());
		UINT particleCount = cloned->GetTotalParticleCount();
		UINT emitterCount = cloned->GetMaxEmitterCount();
		
		PoolHandle handle = RequestAllocation(particleCount, emitterCount, spawnPosCount);
		
		// 할당 실패 시 대기 큐에 추가하고 nullptr 반환
		if (!handle.IsActive()) {
			return nullptr; // 또는 nullptr 반환하여 호출자에게 알림
		}
		
		cloned->SetPoolHandle(handle);

		// SpawnPositions 업로드
		if (!initialData.spawnPositions.empty() && handle.spawnPosOffset != UINT_MAX) {
			m_memoryPool->UploadSpawnPositions(handle.spawnPosOffset, initialData.spawnPositions);
		}

		cloned->InitializeGPU(initialData, 
			m_memoryPool->GetDispatchArgs(),
			m_memoryPool->GetBillboardArgs(),
			m_memoryPool->GetMeshArgs());

		UploadEmitterIDs(cloned.get(), cloned->GetInitialData());
		m_memoryPool->UploadConsts(handle.emitterIDs, initialData.consts);
		m_memoryPool->UploadFrameConsts(handle.emitterIDs, initialData.frameConsts);

		cloned->Initialize(initialData);
		cloned->OnSpawn();

		auto* clonedPtr = cloned.get();
		m_instances.push_back(std::move(cloned));

		RegisterActiveSystem(clonedPtr);

		return clonedPtr;
	}
	
	void ParticleManager::DestroyInstance(ParticleSystem* system)
	{
		if (!system) return;

		// Pool에서 메모리 해제
		const PoolHandle& handle = system->GetPoolHandle();
		if (handle.IsActive()) {
			m_memoryPool->Free(handle);
		}

		// active 목록에서 제거
		UnregisterActiveSystem(system);

		// instance 목록에서 제거
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
			// 1. 기본 통계 (Progress Bar)
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
	}

	PoolHandle ParticleManager::RequestAllocation(UINT particleCount, UINT emitterCount, UINT spawnPosCount)
	{
		PoolHandle handle = m_memoryPool->Allocate(particleCount, emitterCount, spawnPosCount);

		if (!handle.IsActive()) {
			return handle;
		}

		return handle;
	}

	void ParticleManager::UploadEmitterIDs(ParticleSystem* system, const ParticleInitializer& initialData)
	{
		const PoolHandle& handle = system->GetPoolHandle();
		
		for (size_t i = 0; i < initialData.emitterIDs.size(); ++i) {
			EmitterID eID = initialData.emitterIDs[i];
			
			eID.emitterID = handle.emitterIDs[i];
			eID.readParticleOffset = handle.particleOffset + initialData.emitterIDs[i].readParticleOffset;
			eID.writeParticleOffset = handle.particleOffset + initialData.emitterIDs[i].writeParticleOffset;
			
			// spawnPos를 사용하는 emitter만 오프셋 적용
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