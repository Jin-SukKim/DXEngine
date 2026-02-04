#include "pch.h"
#include "ParticleManager.h"
#include "ParticleLoader.h"
#include "RenderBase.h"

namespace DE {
	void ParticleManager::Initialize()
	{
		m_memoryPool = std::make_unique<ParticleMemoryPool>();
		m_memoryPool->Initialize(10000000, 10000, 1000);
	}

	void ParticleManager::Update(const float& dt)
	{
		std::vector<uint32_t> pageTableData;
		pageTableData.reserve(m_memoryPool->GetBlockCount());

		for (auto* system : m_activeSystems) {
			if (!system)
				continue;

			UINT currentOffset = static_cast<UINT>(pageTableData.size());
			system->SetPageTableOffset(currentOffset);

			// Block Index LIst 추가
			const std::vector<UINT>& blocks = system->GetPoolHandle().particleIndices;
			pageTableData.insert(pageTableData.end(), blocks.begin(), blocks.end());

			// EmitterID 업로드 (Manager에서 처리)
			if (system->IsPageTableDirty()) {
				UploadEmitterIDs(system, system->GetInitialData());
				system->ClearPageTableDirty();
			}
		}

		if (!pageTableData.empty())
			m_memoryPool->UploadPageTable(pageTableData);

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
		cloned->InitializeCPU(initialData, m_memoryPool->GetBlockSize());

		UINT spawnPosCount = static_cast<UINT>(initialData.spawnPositions.size());
		UINT blockCount = cloned->GetTotalBlockCount();
		UINT emitterCount = cloned->GetMaxEmitterCount();
		
		PoolHandle handle = RequestAllocation(blockCount, emitterCount, spawnPosCount);
		
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
			return handle;
		}

		return handle;
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