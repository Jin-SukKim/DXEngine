#include "pch.h"
#include "ParticleManager.h"
#include "ParticleLoader.h"
#include "RenderBase.h"

namespace DE {
	void ParticleManager::Initialize()
	{
		m_memoryPool = std::make_unique<ParticleMemoryPool>();
		m_memoryPool->Initialize(10000000, 100);
	}

	void ParticleManager::Update(const float& dt)
	{
		m_memoryPool->ClearWriteCount();
		m_memoryPool->BindCompute();

		for (auto* system : m_activeSystems) {
			if (system) {
				std::vector<ParticleFrameConsts> fsConsts(system->GetMaxEmitterCount());
				system->PreUpdate(dt, fsConsts);
				m_memoryPool->UploadFrameConsts(system->GetPoolHandle().emitterID, fsConsts);
			}
		}

		// 활성 시스템만 업데이트
		for (auto* system : m_activeSystems) {
			if (system) {
				system->Update(dt, m_memoryPool->GetDispatchArgs());
			}
		}

		m_memoryPool->UpdateArgs();

		m_memoryPool->UnbindCompute();

		if (m_memoryPool)
			m_memoryPool->SwapBuffer();
	}

	void ParticleManager::Render()
	{
		if (m_activeSystems.empty()) return;

		m_memoryPool->BindRender();
		// 파티클 렌더링을 위한 PSO 설정은 RenderModule에서 처리
		// 여기서는 순회만 진행
		for (auto* system : m_activeSystems) {
			if (system) {
				system->Render(m_memoryPool->GetBillboardArgs(), m_memoryPool->GetMeshArgs());
			}
		}
		m_memoryPool->UnbindRender();
		// 렌더링 후 BasicPSO로 복원
		GET_SINGLE(RenderBase)->SetPipelineState(RenderBase::graphicsCommon.basic.solidPSO);
	}

	void ParticleManager::RegisterActiveSystem(ParticleSystem* system)
	{
		if (!system) return;
		
		// 중복 체크
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
			m_memoryPool->Free((*it)->GetPoolHandle());
			m_activeSystems.erase(it);
		}
	}

	ParticleSystem* ParticleManager::CreateSystem(const std::wstring& path)
	{
		// 프로토타입 찾기
		auto prototypeIt = m_prototypes.find(path);
		ParticleSystem* prototype = nullptr;

		if (prototypeIt != m_prototypes.end()) {
			// 캐시 히트
			prototype = prototypeIt->second.get();
			//printf("[ParticleManager] Cache HIT: %ls\n", path.c_str());
		}
		else {
			// 캐시 미스 - 원본 로드
			//printf("[ParticleManager] Cache MISS: Loading %ls\n", path.c_str());

			auto newSystem = ParticleLoader::Load<ParticleSystem>(path);
			if (!newSystem) {
				//printf("[ParticleManager] Failed to load: %ls\n", path.c_str());
				return nullptr;
			}

			// 원본 초기화
			newSystem->Initialize();
			//newSystem->OnSpawn();

			prototype = newSystem.get();
			m_prototypes[path] = std::move(newSystem);
		}

		// 복제본 생성
		auto cloned = std::make_unique<ParticleSystem>(*prototype);

		ParticleInitializer initialData;
		// 복제본 초기화 
		cloned->InitializeCPU(initialData);

		PoolHandle handle = RequestAllocation(cloned.get(), cloned->GetTotalParticleCount(), cloned->GetMaxEmitterCount());
		cloned->SetPoolHandle(handle);

		cloned->InitializeGPU(initialData);

		m_memoryPool->UploadConsts(handle.emitterID, initialData.consts);
		m_memoryPool->UploadFrameConsts(handle.emitterID, initialData.frameConsts);

		cloned->Initialize(initialData);
		cloned->OnSpawn();

		auto* clonedPtr = cloned.get();
		m_instances.push_back(std::move(cloned));

		// 자동으로 활성 시스템에 등록
		RegisterActiveSystem(clonedPtr);

		//printf("[ParticleManager] CreateInstance: %p (Cloned)\n", clonedPtr);

		return clonedPtr;
	}
	
	void ParticleManager::DestroyInstance(ParticleSystem* system)
	{
		if (!system) return;

		// 1. 활성 리스트에서 제거 (렌더링/업데이트 제외)
		UnregisterActiveSystem(system);

		// 2. 소유권 리스트(m_instances)에서 찾아 제거
		// erase가 호출되는 순간 unique_ptr이 소멸되며 자동으로 delete가 호출됩니다.
		auto it = std::find_if(m_instances.begin(), m_instances.end(),
			[system](const std::unique_ptr<ParticleSystem>& ptr) {
				return ptr.get() == system;
			});

		if (it != m_instances.end()) {
			// 여기서 실제로 메모리가 해제됩니다.
			m_instances.erase(it);
			//printf("[ParticleManager] Destroyed Instance: %p\n", system);
		}
	}
	PoolHandle ParticleManager::RequestAllocation(ParticleSystem* system, UINT particleCount, UINT emitterCount)
	{
		PoolHandle handle = m_memoryPool->Allocate(particleCount, emitterCount);

		if (!handle.IsActive()) {
			// 메모리 재배치
			m_memoryPool->PlanDefragmentation(m_activeSystems);
			m_waitForSpawn.push(system); // 다음 프레임에 allocate 요청
		}

		return handle;
	}
}