#pragma once
#include "ParticleSystem.h"
#include "ParticleMemoryPool.h"
#include <queue>

namespace DE {
	enum class BlendMode;

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

	struct EmitterJob {
		ParticleEmitter* emitter;
		UINT globalEmitterID; // GPU Buffer offset 계산용
		int materialKey;
		int modelIndex;
		Vector3 worldPosition;  // 시스템 월드 위치 캐싱 (정렬 최적화)
	};

	struct BatchGroup {
		int materialKey; // Material Key
		int modelIndex; // 모델 Key
		BlendMode blendMode;
		std::vector<UINT> emitterIDs;
		UINT instanceOffset;
	};

	struct SortParams {
		UINT baseOffset;
		UINT particleCount;
		UINT padding[2];
		Vector3 cameraForward;
		float padding2;
	};

	// [Added] Runtime Profiling Data Structure
	struct RuntimeProfile {
		float update = 0.0f;
		float update_prepare = 0.0f;
		float update_prepare_setup = 0.0f;
		float update_prepare_cpu = 0.0f;
		float update_prepare_upload = 0.0f;
		float update_args = 0.0f;
		float update_dispatch = 0.0f;
		float update_swap = 0.0f;
		float render = 0.0f;
		float destroy = 0.0f;
		float defrag = 0.0f;
		float requestAlloc = 0.0f;
		float uploadIDs = 0.0f;
		float recalculateOffsets = 0.0f;
		float syncReadOffsets = 0.0f;
		float eviction = 0.0f;

		// [Added] Frustum Culling Stats
		UINT totalSystems = 0;
		UINT visibleSystems = 0;
		UINT culledSystems = 0;

		// [Added] Eviction Stats
		UINT evictionCount = 0;
		UINT evictionFailures = 0;

		void Reset() {
			update = 0.0f;
			update_prepare = 0.0f;
			update_prepare_setup = 0.0f;
			update_prepare_cpu = 0.0f;
			update_prepare_upload = 0.0f;
			update_args = 0.0f;
			update_dispatch = 0.0f;
			update_swap = 0.0f;
			render = 0.0f;
			destroy = 0.0f;
			defrag = 0.0f;
			requestAlloc = 0.0f;
			uploadIDs = 0.0f;
			recalculateOffsets = 0.0f;
			syncReadOffsets = 0.0f;
			eviction = 0.0f;
			totalSystems = 0;
			visibleSystems = 0;
			culledSystems = 0;
			evictionCount = 0;
			evictionFailures = 0;
		}
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

		void BuildBatches(const std::vector<EmitterJob>& jobs, std::vector<BatchGroup>& outBatches);
		void BuildMeshBatches(const std::vector<EmitterJob>& jobs, std::vector<BatchGroup>& outBatches);

		void RenderDepth();

		// Frustum Culling용
		void SetViewAndProj(const Matrix& view, const Matrix& proj) {
			m_view = view;
			m_proj = proj;
		}

		void RegisterActiveSystem(ParticleSystem* system);
		void UnregisterActiveSystem(ParticleSystem* system);

		ParticleSystem* CreateSystem(const std::wstring& path);
		void DestroyInstance(ParticleSystem* system);

		// EmitterID ε (Manager ó)
		void BindEmitterID(UINT globalSlotIndex);

		// MeshConsts
		void UpdateMeshConsts(UINT systemIndex, const ParticleMeshConsts& data);

		// Debug
		void RenderMemoryPoolGUI();
		void Defragment();

		UINT GetRebuildCount() const { return m_rebuildCount; }
		float GetAvgRebuildTime() const { return m_avgRebuildTime; }

		// Memory Pool 접근
		ParticleMemoryPool& GetMemoryPool() { return *m_memoryPool; }

	private:
		// Render sub-steps
		void GatherVisibleEmitters(const DirectX::BoundingFrustum& frustum,
		                           const Vector3& cameraPos,
		                           UINT& totalCount, UINT& visibleCount);
		void BuildBatchDescriptors();
		void DispatchBatchCompute();
		void SortAlphaBlendEmitters();
		void DrawMeshBatches();
		void DrawMeshBatchesDepth();
		void DrawBillboardBatches();

		PoolHandle RequestAllocation(UINT particleCount, UINT emitterCount, UINT spawnPosCount);
		void UploadEmitterIDs(ParticleSystem* system, const ParticleInitializer& initialData);
		void RecalculateEmitterOffsets(ParticleSystem* system, UINT newParticleOffset);

		void SyncReadOffsets();

		void ResetMetrics();

		// [Added] EMA Helper for smooth metric updates
		void UpdateMetric(float& metric, float newValue) {
			if (metric == 0.f) metric = newValue;
			else metric = metric * 0.9f + newValue * 0.1f;
		}

		// Priority-based eviction helpers
		float CalculatePriority(ParticleSystem* system, const Vector3& cameraPos,
			const DirectX::BoundingFrustum& frustum) const;
		ParticleSystem* FindLowestPrioritySystem(UINT particleCount);
		bool TryEvictAndRetry(UINT particleCount, UINT emitterCount, UINT spawnPosCount,
			PoolHandle& outHandle);

	private:
		std::unordered_map<std::wstring, std::unique_ptr<ParticleSystem>> m_prototypes;
		std::vector<std::unique_ptr<ParticleSystem>> m_instances;
		// [0] = Mesh RenderModule, [1] = Billboard RenderModule
		std::vector<ParticleSystem*> m_activeSystems;

		std::unique_ptr<ParticleMemoryPool> m_memoryPool;

		bool m_needsDefragment = false;
		bool m_needsSyncReadOffset = false;

		UINT m_rebuildCount = 0;
		float m_totalRebuildTime = 0.0f;
		float m_avgRebuildTime = 0.0f;

		// [Added] Profiling Data
		RuntimeProfile m_runtimeProfile;

		// [Added] Frustum Culling
		Matrix m_view;
		Matrix m_proj;

		// [Added] Priority-based eviction time tracking
		float m_currentTime = 0.0f;

		// [Added] Batch rendering
		std::vector<BatchGroup> m_billboardBatches;
		std::vector<BatchGroup> m_meshBatches;

		// CS용 ConstantBuffer (Initialize에서 1회 초기화)
		ConstantBuffer<BatchRenderArgsConsts> m_batchRenderArgsCB;
		ConstantBuffer<BuildAliveConsts> m_buildAliveCB;
		ConstantBuffer<SortParams> m_sortParamsCB;

		// 매 프레임 재사용 컨테이너
		std::vector<EmitterJob> m_meshJobs;
		std::vector<EmitterJob> m_billboardJobs;
		std::vector<UINT> m_batchEmitterList;
		std::vector<BatchDescriptor> m_batchDescriptors;
		UINT m_billboardDescStartIdx = 0;
	};

}
