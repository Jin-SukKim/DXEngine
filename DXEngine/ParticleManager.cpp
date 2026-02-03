#include "pch.h"
#include "ParticleManager.h"
#include "ParticleLoader.h"
#include "RenderBase.h"

namespace DE {
	void ParticleManager::Initialize()
	{
		m_memoryPool = std::make_unique<ParticleMemoryPool>();
		m_memoryPool->Initialize(10000, 10000, 1000);
	}

	void ParticleManager::Update(const float& dt)
	{
		m_memoryPool->ClearWriteCount();
		m_memoryPool->BindCompute();

		// 유효한 시스템만 처리
		for (auto* system : m_activeSystems) {
			if (system && system->GetPageHandle().IsActive()) {
				std::vector<ParticleFrameConsts> fsConsts(system->GetMaxEmitterCount());
				system->PreUpdate(dt, fsConsts);
				m_memoryPool->UploadFrameConsts(system->GetPageHandle().emitterIDs, fsConsts);
			}
		}

		m_memoryPool->UpdateArgs();

		for (auto* system : m_activeSystems) {
			if (system && system->GetPageHandle().IsActive()) {
				m_memoryPool->BindMeshConsts(system->GetPageHandle().systemSlot);
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
			if (system && system->GetPageHandle().IsActive()) {
				m_memoryPool->BindMeshConsts(system->GetPageHandle().systemSlot);
				system->Render();
			}
		}
		m_memoryPool->UnbindRender();
		GET_SINGLE(RenderBase)->SetPipelineState(RenderBase::graphicsCommon.basic.solidPSO);

		ProcessWaitingQueue();
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
		
		PageHandle handle = m_memoryPool->Allocate(particleCount, emitterCount, spawnPosCount);
		
		// 할당 실패 시 대기 큐에 추가
		if (!handle.IsActive()) {
			std::cout << "Failed to Create ParticleSystem (Out of pages), adding to queue" << std::endl;
			
			PendingSystem pending;
			pending.system = cloned.get();
			pending.particleCount = particleCount;
			pending.emitterCount = emitterCount;
			pending.spawnPosCount = spawnPosCount;
			pending.retryCount = 0;
			m_waitForSpawn.push(pending);
			
			// instance는 저장하되 active에는 등록하지 않음
			auto* clonedPtr = cloned.get();
			m_instances.push_back(std::move(cloned));
			return clonedPtr;
		}
		
		cloned->SetPageHandle(handle);

		// SpawnPositions 업로드 (비연속 페이지)
		if (!initialData.spawnPositions.empty() && !handle.spawnPosPageIndices.empty()) {
			m_memoryPool->UploadSpawnPositions(
				handle.spawnPosPageIndices, 
				initialData.spawnPositions
			);
		}

		cloned->InitializeGPU(initialData, 
			m_memoryPool->GetDispatchArgs(),
			m_memoryPool->GetBillboardArgs(),
			m_memoryPool->GetMeshArgs());

		// EmitterID 업로드 (Manager에서 처리)
		UploadEmitterIDs(cloned.get(), initialData);
			
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

		// 1. active 목록에서 먼저 제거
		UnregisterActiveSystem(system);

		// 2. Pool에서 메모리 해제
		const PageHandle& handle = system->GetPageHandle();
		if (handle.IsActive()) {
			m_memoryPool->Free(handle);
		}

		// 3. 대기 큐에서도 제거 (있다면)
		// 큐를 순회하며 해당 시스템 제거
		std::queue<PendingSystem> tempQueue;
		while (!m_waitForSpawn.empty()) {
			PendingSystem pending = m_waitForSpawn.front();
			m_waitForSpawn.pop();
			if (pending.system != system) {
				tempQueue.push(pending);
			}
		}
		m_waitForSpawn = std::move(tempQueue);

		// 4. instance 목록에서 제거
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

	void ParticleManager::UploadEmitterIDs(ParticleSystem* system, const ParticleInitializer& initialData)
	{
		const PageHandle& handle = system->GetPageHandle();
		
		for (size_t i = 0; i < initialData.emitterIDs.size(); ++i) {
			EmitterIDPaged eID;
			eID.emitterID = handle.emitterIDs[i];
			eID.pageTableStart = handle.pageIndices.empty() ? 0 : handle.pageIndices[0];
			eID.pageCount = handle.pageCount;
			eID.localParticleMax = handle.totalCapacity / std::max(1u, handle.emitterCount);
			
			// SpawnPos 페이지 정보 설정
			if (!handle.spawnPosPageIndices.empty()) {
				eID.spawnPosPageTableStart = handle.spawnPosPageIndices[0];
				eID.spawnPosPageCount = handle.spawnPosPageCount;
			}
			else {
				eID.spawnPosPageTableStart = INVALID_PAGE;
				eID.spawnPosPageCount = 0;
			}
			
			m_memoryPool->UpdateEmitterID(handle.emitterIDs[i], eID);
		}
	}

	void ParticleManager::ProcessWaitingQueue()
	{
		if (m_waitForSpawn.empty()) return;

		constexpr int MAX_RETRY = 3;
		size_t qSize = m_waitForSpawn.size();

		for (size_t i = 0; i < qSize; ++i) {
			PendingSystem pending = m_waitForSpawn.front();
			m_waitForSpawn.pop();

			// Validate System
			auto it = std::find_if(m_instances.begin(), m_instances.end(),
				[&pending](const std::unique_ptr<ParticleSystem>& ptr) {
					return ptr.get() == pending.system;
				});

			if (it == m_instances.end()) {
				continue; // System already deleted, skip
			}

			// Try Allocation
			PageHandle handle = m_memoryPool->Allocate(
				pending.particleCount,
				pending.emitterCount,
				pending.spawnPosCount
			);

			if (handle.IsActive()) {
				ParticleSystem* system = pending.system;
				system->SetPageHandle(handle);

				ParticleInitializer initialData;
				system->InitializeCPU(initialData);

				if (!initialData.spawnPositions.empty() && !handle.spawnPosPageIndices.empty()) {
					m_memoryPool->UploadSpawnPositions(
						handle.spawnPosPageIndices,
						initialData.spawnPositions
					);
				}

				system->InitializeGPU(initialData,
					m_memoryPool->GetDispatchArgs(),
					m_memoryPool->GetBillboardArgs(),
					m_memoryPool->GetMeshArgs());

				UploadEmitterIDs(system, initialData);
				m_memoryPool->UploadConsts(handle.emitterIDs, initialData.consts);
				m_memoryPool->UploadFrameConsts(handle.emitterIDs, initialData.frameConsts);

				system->Initialize(initialData);
				system->OnSpawn();

				RegisterActiveSystem(system);
				std::cout << "Success: Spawned Pending System" << std::endl;
			}
			else {
				pending.retryCount++;
				if (pending.retryCount >= MAX_RETRY) {
					std::cout << "Failed to allocate pending system after " << MAX_RETRY << " retries. Dropping it." << std::endl;
					
					// 중요: m_activeSystems에서도 제거 (혹시 등록되어 있다면)
					UnregisterActiveSystem(pending.system);
					
					// m_instances에서 제거
					m_instances.erase(it);
				}
				else {
					// 다시 큐에 넣음
					m_waitForSpawn.push(pending);
				}
			}
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