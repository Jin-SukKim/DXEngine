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
	
	// Debug
	void RenderMemoryPoolGUI();
	void Defragment();

	UINT GetRebuildCount() const { return m_rebuildCount; }
	float GetAvgRebuildTime() const { return m_avgRebuildTime; }
private:
	PoolHandle RequestAllocation(UINT particleCount, UINT emitterCount, UINT spawnPosCount);
	void UploadEmitterIDs(ParticleSystem* system, const ParticleInitializer& initialData);
	void RecalculateEmitterOffsets(ParticleSystem* system, UINT newParticleOffset);

	void SyncReadOffsets();

	void ResetMetrics();

private:
	std::unordered_map<std::wstring, std::unique_ptr<ParticleSystem>> m_prototypes;
	std::vector<std::unique_ptr<ParticleSystem>> m_instances;
	std::vector<ParticleSystem*> m_activeSystems;

	std::unique_ptr<ParticleMemoryPool> m_memoryPool;

	// 멤버 변수 추가
	bool m_needsDefragment = false;
	bool m_needsSyncReadOffset = false;

	// private 멤버 추가
	UINT m_rebuildCount = 0;
	float m_totalRebuildTime = 0.0f;
	float m_avgRebuildTime = 0.0f;

	UINT m_createCount = 0;
	float m_totalCreateTime = 0.f;
	float m_avgCreateTime = 0.f;

	float m_totalPrototypeTime = 0.f;
	float m_avgPrototypeTime = 0.f;

	UINT m_cacheHitCount = 0;
	UINT m_cacheMissCount = 0;
	float m_totalCacheSearchTime = 0.0f;
	float m_avgCacheSearchTime = 0.0f;
	float m_totalFileLoadTime = 0.0f;
	float m_avgFileLoadTime = 0.0f;

	float m_totalCloneTime = 0.f;
	float m_avgCloneTime = 0.f;

	float m_totalInitalizeTime = 0.f;
	float m_avgInitializeTime = 0.f;

	float m_totalAllocTime = 0.f;
	float m_avgAllocTime = 0.f;

	float m_totalGpuTime = 0.f;
	float m_avgGpuTime = 0.f;

	UINT m_failCount = 0;           // 생성 실패 횟수
	float m_totalFailTime = 0.f;    // 실패하는 데 걸린 총 시간 (낭비된 시간)
	float m_totalSetupTime = 0.0f;      // InitializeCPU 시간 합계
	float m_avgSetupTime = 0.0f;        // InitializeCPU 시간 평균

	float m_totalPoolAllocTime = 0.0f;  // RequestAllocation 시간 합계
	float m_avgPoolAllocTime = 0.0f;    // RequestAllocation 시간 평균

	float m_totalInitMainTime = 0.0f;
	float m_avgInitMainTime = 0.0f;

	float m_totalInitSubTime = 0.0f;
	float m_avgInitSubTime = 0.0f;
};

}

