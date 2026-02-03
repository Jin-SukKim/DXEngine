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
		CompactParticleOffset();

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

		FinishDefragmentation();

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
		
		PoolHandle handle = RequestAllocation(particleCount, emitterCount, spawnPosCount);
		
		// 할당 실패 시 대기 큐에 추가하고 nullptr 반환
		if (!handle.IsActive()) {
			std::cout << "Failed to Create ParticelSystem" << std::endl;
			PendingSystem pending;
			pending.system = cloned.get();
			pending.particleCount = particleCount;
			pending.emitterCount = emitterCount;
			pending.spawnPosCount = spawnPosCount;
			m_waitForSpawn.push(pending);
			m_needCompact = true;
			
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

	PoolHandle ParticleManager::RequestAllocation(UINT particleCount, UINT emitterCount, UINT spawnPosCount)
	{
		PoolHandle handle = m_memoryPool->Allocate(particleCount, emitterCount, spawnPosCount);

		if (!handle.IsActive()) {
			m_memoryPool->PlanDefragmentation(m_activeSystems);
		}

		return handle;
	}

	void ParticleManager::UploadEmitterIDs(ParticleSystem* system, const ParticleInitializer& initialData)
	{
		const PoolHandle& handle = system->GetPoolHandle();
		
		for (size_t i = 0; i < initialData.emitterIDs.size(); ++i) {
			EmitterID eID = initialData.emitterIDs[i];
			
			eID.emitterID = handle.emitterIDs[i];
			eID.readParticleOffset += handle.particleOffset;
			eID.writeParticleOffset += handle.particleOffset;
			
			// spawnPos를 사용하는 emitter만 오프셋 적용
			if (handle.spawnPosOffset != UINT_MAX && eID.spawnPosOffset != UINT_MAX) {
				eID.spawnPosOffset += handle.spawnPosOffset;
			}
			
			m_memoryPool->UpdateEmitterID(handle.emitterIDs[i], eID);
		}
	}

	void ParticleManager::ProcessWaitingQueue()
	{
		if (m_waitForSpawn.empty()) return;

		// Do not process queue while defragmentation is in progress or scheduled
		if (m_needCompact || m_memoryPool->IsDefragStarted())
			return;

		std::cout << "Processing Waiting Queue" << std::endl;

		// Safety mechanism: process only current items to avoid infinite same-frame loops
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
				continue; // System deleted, skip
			}

			// Try Allocation
			PoolHandle handle = m_memoryPool->Allocate(
				pending.particleCount,
				pending.emitterCount,
				pending.spawnPosCount
			);

			if (handle.IsActive()) {
				ParticleSystem* system = pending.system;
				system->SetPoolHandle(handle);

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
				m_memoryPool->UploadConsts(handle.emitterIDs, initialData.consts);
				m_memoryPool->UploadFrameConsts(handle.emitterIDs, initialData.frameConsts);

				system->Initialize(initialData);
				system->OnSpawn();

				RegisterActiveSystem(system);
				std::cout << "Success: Spawned Pending System" << std::endl;
			}
			else {
				std::cout << "Failed to allocate pending system (OOM). Dropping it." << std::endl;

				m_instances.erase(it);
			}
		}
	}

	void ParticleManager::CompactParticleOffset()
	{
		if (!m_needCompact && !m_memoryPool->IsDefragStarted())
			return;

		std::cout << "Start Compacting" << std::endl;
		for (auto* ps : m_activeSystems) {
			PoolHandle cur = ps->GetPoolHandle();
			PoolHandle next = ps->GetNextPoolHandle();

			if (cur == next)
				continue;

			UINT emitterCount = ps->GetMaxEmitterCount();
			for (UINT i = 0; i < emitterCount; ++i)
				m_memoryPool->UpdateEmitterID(cur.emitterIDs[i], next, i);
		}
	}

	void ParticleManager::FinishDefragmentation()
	{
		// Defrag가 진행된 프레임이 아니면 무시
		// (CompactParticleOffset에서 m_needCompact를 끄지 않도록 주의해야 함.
		//  여기서 끄는 게 맞음.)
		if (!m_memoryPool->IsDefragStarted())
			return;

		// 이사 간 시스템들 찾아서 주소지 변경
		for (auto* ps : m_activeSystems) {
			PoolHandle cur = ps->GetPoolHandle();
			PoolHandle next = ps->GetNextPoolHandle();

			// 이사 간 경우 (주소가 다름)
			if (cur.particleOffset != next.particleOffset) {

				// 1. 시스템의 핸들을 새 주소로 확정 (Commit)
				ps->SetPoolHandle(next);

				// 2. 쉐이더 상수 버퍼(EmitterID) 업데이트
				// 중요: 이제 "새 주소"에서 읽어야 하므로 ReadOffset도 업데이트!
				const auto& slots = next.emitterIDs; // [변경됨] next의 emitterIDs 사용

				for (size_t i = 0; i < slots.size(); ++i) {
					EmitterID eID;
					// Read와 Write 모두 새 주소를 가리키게 설정
					eID.emitterID = slots[i];
					eID.readParticleOffset = next.particleOffset;  // [중요] New
					eID.writeParticleOffset = next.particleOffset; // [중요] New

					// SpawnPos도 이사 갔다면 업데이트 (생략 가능하면 생략)
					if (next.spawnPosOffset != UINT_MAX) {
						eID.spawnPosOffset = next.spawnPosOffset;
					}

					// 상수 버퍼 갱신
					m_memoryPool->UpdateEmitterID(slots[i], eID);
				}
			}
		}

		// 모든 처리가 끝났으므로 플래그 해제
		m_needCompact = false;
		m_memoryPool->FinishDefrag();
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