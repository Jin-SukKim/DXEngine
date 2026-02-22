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
		UINT sortBaseOffset;
		UINT sortParticleCount;
		UINT padding[2];
		Vector3 cameraForward;
		float padding2;
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

		void Defragment();

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
