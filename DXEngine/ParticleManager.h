#pragma once
#include "ParticleSystem.h"
#include "ParticleMemoryPool.h"
#include <queue>

namespace DE {

struct ParticlePreset {
	std::unique_ptr<ParticleSystem> prototype;
	std::wstring filePath;
	FileWatcher::CallbackID watcherID = 0;
};

struct PendingSystem {
	ParticleSystem* system = nullptr;
	UINT particleCount = 0;
	UINT emitterCount = 0;
	UINT spawnPosCount = 0;
	int retryCount = 0;
};

class ParticleManager
{
public:
	static ParticleManager& Get() {
		static ParticleManager instance;
		return instance;
	}

	void Initialize();
	void Update(const float& dt);
	void Render();
	
	void RegisterActiveSystem(ParticleSystem* system);
	void UnregisterActiveSystem(ParticleSystem* system);

	ParticleSystem* CreateSystem(const std::wstring& path);
	void DestroyInstance(ParticleSystem* system);

	// EmitterID 바인딩 (Manager에서 처리)
	void BindEmitterID(UINT globalSlotIndex);
	
	// MeshConsts 관리 추가
	void UploadMeshConsts(UINT systemIndex, const MeshConstants& data);
	void BindMeshConsts(UINT systemIndex);

private:
	PoolHandle RequestAllocation(UINT particleCount, UINT emitterCount, UINT spawnPosCount);
	void UploadEmitterIDs(ParticleSystem* system, const ParticleInitializer& initialData);

private:
	std::unordered_map<std::wstring, std::unique_ptr<ParticleSystem>> m_prototypes;
	std::vector<std::unique_ptr<ParticleSystem>> m_instances;
	std::vector<ParticleSystem*> m_activeSystems;

	std::unique_ptr<ParticleMemoryPool> m_memoryPool;
};

}

