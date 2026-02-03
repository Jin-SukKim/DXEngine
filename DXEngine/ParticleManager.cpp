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
		CompactParticleOffset();

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

		FinishDefragmentation();
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
			
			eID.readEmitterID += handle.emitterID;
			eID.writeEmitterID += handle.emitterID;
			eID.readParticleOffset += handle.particleOffset;
			eID.writeParticleOffset += handle.particleOffset;
			
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

	void ParticleManager::CompactParticleOffset()
	{
		if (!m_needCompact && !m_memoryPool->IsDefragStarted())
			return;

		for (auto* ps : m_activeSystems) {
			PoolHandle cur = ps->GetPoolHandle();
			PoolHandle next = ps->GetNextPoolHandle();

			if (cur == next)
				continue;

			UINT emitterCount = ps->GetMaxEmitterCount();
			for (UINT i = 0; i < emitterCount; ++i)
				m_memoryPool->UpdateEmitterID(cur.emitterID + i, next, i);
		}
	}

	void ParticleManager::FinishDefragmentation()
	{
		if (!m_needCompact && !m_memoryPool->IsDefragStarted())
			return;

		ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();

		// 1. 데이터 다운로드 및 준비
		StructuredBuffer<ParticleFrameConsts>& pfConsts = m_memoryPool->GetFrameConsts();
		pfConsts.Download(context);
		std::vector<ParticleFrameConsts> pfConstsCPU(pfConsts.Size());

		StructuredBuffer<ParticleConsts>& pConsts = m_memoryPool->GetConsts();
		pConsts.Download(context);
		std::vector<ParticleConsts> pConstsCPU(pConsts.Size());

		StructuredBuffer<Vector3>& pSpawn = m_memoryPool->GetSpawnPosBuffer();
		pSpawn.Download(context);
		auto& pSpawnPos = pSpawn.GetCpu(); // 원본(구) 데이터
		std::vector<Vector3> pSpawnCPU(pSpawn.Size()); // 타겟(신) 데이터 버퍼 (0으로 초기화됨)

		std::vector<ConstantBuffer<EmitterID>>& pIDs = m_memoryPool->GetEmitterIDs();
		// EmitterID는 객체별로 관리되므로 CPU 값만 복사하면 됨 (별도 벡터 불필요, 직접 pIDs에 set)

		// 2. 재배치 루프
		for (auto* ps : m_activeSystems) {
			PoolHandle cur = ps->GetPoolHandle();
			PoolHandle next = ps->GetNextPoolHandle();

			if (cur == next) continue;

			UINT emitterCount = ps->GetMaxEmitterCount();

			// A. Emitter 관련 데이터 이동
			for (UINT i = 0; i < emitterCount; ++i) {
				UINT oldIdx = cur.emitterID + i;
				UINT newIdx = next.emitterID + i;

				// FrameConsts & Consts 복사
				if (oldIdx < pfConsts.Size() && newIdx < pfConsts.Size()) {
					pfConstsCPU[newIdx] = pfConsts.Get(oldIdx);
					pConstsCPU[newIdx] = pConsts.Get(oldIdx);
				}

				// EmitterID 업데이트 (GPU 업로드는 나중에 일괄 혹은 Update 루프에서)
				if (oldIdx < pIDs.size() && newIdx < pIDs.size()) {
					EmitterID eID = pIDs[oldIdx].GetCpu();

					// Commit: Write였던 것을 Read로 승격
					eID.readEmitterID = eID.writeEmitterID;
					eID.readParticleOffset = eID.writeParticleOffset;

					// SpawnPos 오프셋 보정 (있다면)
					if (cur.spawnPosOffset != UINT_MAX && next.spawnPosOffset != UINT_MAX) {
						// 로컬 오프셋 = 현재 절대 오프셋 - 현재 베이스 오프셋
						UINT localSpawnOffset = eID.spawnPosOffset - cur.spawnPosOffset;
						eID.spawnPosOffset = next.spawnPosOffset + localSpawnOffset;
					}

					pIDs[newIdx].SetCpuData(eID); // 새 위치에 데이터 세팅
				}
			}

			// B. SpawnPos 데이터 이동
			if (cur.spawnPosOffset != UINT_MAX && next.spawnPosOffset != UINT_MAX) {
				UINT copySize = cur.spawnPosBlockCount * m_memoryPool->GetBlockSize();
				// 범위 체크 후 복사
				if (cur.spawnPosOffset + copySize <= pSpawnPos.size() &&
					next.spawnPosOffset + copySize <= pSpawnCPU.size())
				{
					std::copy(
						pSpawnPos.begin() + cur.spawnPosOffset,
						pSpawnPos.begin() + cur.spawnPosOffset + copySize,
						pSpawnCPU.begin() + next.spawnPosOffset
					);
				}
			}

			// C. 시스템 핸들 확정 (중요!)
			ps->SetPoolHandle(next);
		}

		// 3. GPU 업로드
		pfConsts.SetData(pfConstsCPU);
		pfConsts.Upload(context);

		pConsts.SetData(pConstsCPU);
		pConsts.Upload(context);

		pSpawn.SetData(pSpawnCPU);
		pSpawn.Upload(context);

		// EmitterID 일괄 업로드 (선택사항, 보통 다음 Update에서 처리됨)
		for (auto& cb : pIDs) {
			if (cb.GetCpu().readEmitterID != UINT_MAX) // 유효한 것만
				cb.Upload();
		}

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