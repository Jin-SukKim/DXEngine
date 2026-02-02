#include "pch.h"
#include "ParticleManager.h"
#include "ParticleLoader.h"
#include "RenderBase.h"

namespace DE {
	void ParticleManager::Initialize()
	{
		m_memoryPool = std::make_unique<ParticleMemoryPool>();
		m_memoryPool->Initialize(10000000, 100, 100);
	}

	void ParticleManager::Update(const float& dt)
	{
		ProcessWaitingQueue();

		m_memoryPool->ClearWriteCount();
		m_memoryPool->BindCompute();

		for (auto* system : m_activeSystems) {
			if (system) {
				std::vector<ParticleFrameConsts> fsConsts(system->GetMaxEmitterCount());
				system->PreUpdate(dt, fsConsts);
				m_memoryPool->UploadFrameConsts(system->GetPoolHandle().emitterID, fsConsts);
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
			PendingSystem pending;
			pending.system = cloned.get();
			pending.particleCount = particleCount;
			pending.emitterCount = emitterCount;
			pending.spawnPosCount = spawnPosCount;
			m_waitForSpawn.push(pending);
			
			// instance는 저장하되 active에는 등록하지 않음
			auto* clonedPtr = cloned.get();
			m_instances.push_back(std::move(cloned));
			return clonedPtr; // 또는 nullptr 반환하여 호출자에게 알림
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

		// EmitterID 업로드 (Manager에서 처리)
		UploadEmitterIDs(cloned.get(), initialData);
			
		m_memoryPool->UploadConsts(handle.emitterID, initialData.consts);
		m_memoryPool->UploadFrameConsts(handle.emitterID, initialData.frameConsts);

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

	PoolHandle ParticleManager::RequestAllocation(UINT particleCount, UINT emitterCount, UINT spawnPosCount)
	{
		PoolHandle handle = m_memoryPool->Allocate(particleCount, emitterCount, spawnPosCount);

		if (!handle.IsActive()) {
			m_memoryPool->PlanDefragmentation(m_activeSystems);
		}

		return handle;
	}

	void ParticleManager::UploadEmitterIDs(ParticleSystem* system, ParticleInitializer& initialData)
	{
		const PoolHandle& handle = system->GetPoolHandle();
		
		for (size_t i = 0; i < initialData.emitterIDs.size(); ++i) {
			EmitterID eID = initialData.emitterIDs[i];
			
			eID.emitterID += handle.emitterID;
			eID.particleOffset += handle.particleOffset;
			
			// spawnPos를 사용하는 emitter만 오프셋 적용
			if (handle.spawnPosOffset != UINT_MAX && eID.spawnPosOffset != UINT_MAX) {
				eID.spawnPosOffset += handle.spawnPosOffset;
			}
			
			m_memoryPool->UpdateEmitterID(handle.emitterID + static_cast<UINT>(i), eID);
		}
	}

	void ParticleManager::ProcessWaitingQueue()
	{
		if (m_waitForSpawn.empty()) return;

		while (!m_waitForSpawn.empty()) {
			PendingSystem pending = m_waitForSpawn.front();
			
			// 시스템이 아직 유효한지 확인
			auto it = std::find_if(m_instances.begin(), m_instances.end(),
				[&pending](const std::unique_ptr<ParticleSystem>& ptr) {
					return ptr.get() == pending.system;
				});
			
			if (it == m_instances.end()) {
				// 시스템이 이미 삭제됨
				m_waitForSpawn.pop();
				continue;
			}

			// 재할당 시도
			PoolHandle handle = m_memoryPool->Allocate(
				pending.particleCount, 
				pending.emitterCount, 
				pending.spawnPosCount
			);

			if (!handle.IsActive()) {
				// 할당 실패 시 중단 (메모리 부족)
				break;
			}

			m_waitForSpawn.pop();
			
			ParticleSystem* system = pending.system;
			system->SetPoolHandle(handle);

			// GPU 초기화 및 등록
			ParticleInitializer initialData;
			system->InitializeCPU(initialData);

			if (!initialData.spawnPositions.empty() && handle.spawnPosOffset != UINT_MAX) {
				m_memoryPool->UploadSpawnPositions(handle.spawnPosOffset, initialData.spawnPositions);
			}

			system->InitializeGPU(initialData,
				m_memoryPool->GetDispatchArgs(),
				m_memoryPool->GetBillboardArgs(),
				m_memoryPool->GetMeshArgs());

			UploadEmitterIDs(system, initialData);
			m_memoryPool->UploadConsts(handle.emitterID, initialData.consts);
			m_memoryPool->UploadFrameConsts(handle.emitterID, initialData.frameConsts);

			system->Initialize(initialData);
			system->OnSpawn();

			RegisterActiveSystem(system);
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