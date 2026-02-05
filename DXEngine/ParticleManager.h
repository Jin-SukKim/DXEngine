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
	
	// MeshConsts 관리 추가
	void UploadMeshConsts(UINT systemIndex, const MeshConstants& data);
	
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

	struct RuntimeProfile {
		float update = 0.f;         // Update 전체 시간
		float render = 0.f;
		float destroy = 0.f;
		float defrag = 0.f;

		// ★ Update 세부 항목 추가
		float update_prepare = 0.f; // FrameConsts 업로드 등 준비 시간
		// Prepare 상세 분할
		float update_prepare_setup = 0.f;   // ClearWriteCount, BindCompute
		float update_prepare_cpu = 0.f;     // PreUpdate 루프 (CPU 연산 & 할당)
		float update_prepare_upload = 0.f;  // UploadFrameConsts (GPU 전송)
		
		float update_args = 0.f;    // Indirect Args Update 시간
		float update_dispatch = 0.f;// 실제 Compute Shader Dispatch 시간
		float update_swap = 0.f;    // SwapBuffer 시간

		// Private/Internal helpers
		float requestAlloc = 0.f;
		float uploadIDs = 0.f;
		float recalculateOffsets = 0.f;
		float syncReadOffsets = 0.f;

		void Reset() { *this = RuntimeProfile(); }
	} m_runtimeProfile;

	// EMA 적용 함수
	void UpdateMetric(float& metric, float newValue) {
		if (metric == 0.f) metric = newValue;
		else metric = metric * 0.9f + newValue * 0.1f;
	}
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
};

}

