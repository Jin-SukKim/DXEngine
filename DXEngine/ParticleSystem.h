#pragma once
#include "Object.h"
#include "ParticleEmitter.h"
#include "MeshData.h"
#include "ParticleMemoryPool.h"

namespace DE {
	struct Mesh;

	enum class ParticleState {
		Playing,
		Paused,
		Stopped
	};

	struct ParticleInitializer {
		std::vector<ParticleConsts> consts;
		std::vector<ParticleFrameConsts> frameConsts;
		std::vector<DrawIndexedInstancedArgs> initMeshArgs;
		std::vector<EmitterID> emitterIDs;

		// Baked/Custom 
		std::vector<Vector3> spawnPositions;  //  SpawnPosition
		UINT totalSpawnPosCount = 0;          //  SpawnPosition 
	};

	class ParticleSystem : public Object
	{
	public:
		ParticleSystem(const std::wstring& name);
		~ParticleSystem() override;

		ParticleSystem(const ParticleSystem& other);
		UINT GetEmitterCount() const { return static_cast<UINT>(m_emitters.size()); }

		void Initialize() override;
		void Initialize(ParticleInitializer& initialData);
		void InitializeCPU(ParticleInitializer& initialData);
		void InitializeGPU(ParticleInitializer& initialData,
			IndirectArgsBuffer<DispatchArgs>& m_dispatchArgs,
			IndirectArgsBuffer<DrawIndexedInstancedArgs>& m_billboardArgsBuffer,
			IndirectArgsBuffer<DrawIndexedInstancedArgs>& m_meshArgsBuffer);

		void OnSpawn();
		void PreUpdate(const float& dt, std::vector<ParticleFrameConsts>& fsConsts);
		void Update(const float& dt);
		void ActivateSubEmitters();
		void Render();

		void AddEmitter(const std::string& path);
		void AddEmitter(std::unique_ptr<ParticleEmitter>&& emitter);
		void ClearEmitters();
		void LoadFromJson(const json& data);
		void SetHotReloadInfo(const std::wstring& path, FileWatcher::CallbackID id);
		void ProcessEmitter(
			ParticleEmitter* emitter,
			ParticleInitializer& initialData);

		void Play();
		void Pause();
		void Stop();
		void Restart();

		void SetLooping(bool loop) { m_looping = loop; }
		void SetDuration(float duration) { m_duration = duration; }
		void SetPlayRate(float rate) { m_playRate = rate; }
		void SetPreWarmTime(float time) { m_preWarmTime = time; }
		void SetTargetMesh(const int& modelIdx);

		// Mesh  
		UINT GetVertexCount() const { return m_vertexCount; }
		UINT GetIndexCount() const { return m_indexCount; }

		void SetTransform(const MeshConstants& transform);

		bool IsLooping() { return m_looping; }
		bool IsStopped() const { return m_state == ParticleState::Stopped; }
		bool IsPlaying() const { return m_state == ParticleState::Playing; }
		void SetState(ParticleState state) { m_state = state; }

		void SetTarget(Actor* owner = nullptr, const int& modelIdx = -1);
		void SetSpawnOffset(const Vector3& offset);

		bool IsAllEmittersCompleted() const;
		// RenderModule 타입 확인 (렌더링 순서 최적화용)
		bool HasMeshRenderModule() const;
		bool HasBillboardRenderModule() const;

		UINT GetDispatchArgsOffset(UINT emitterID) { return emitterID * 12; }
		UINT GetBillboardArgsOffset(UINT emitterID) { return emitterID * 20; } // DrawIndexedInstancedArgs (5 * 4 bytes)
		UINT GetMeshArgsOffset(UINT emitterID) { return emitterID * 20; }

		void BindConstantID(UINT emitterID);

		UINT GetTotalParticleCount() const { return m_maxTotalParticles; }
		UINT GetMaxEmitterCount() const { return m_maxEmitters; }
		void SetPoolHandle(PoolHandle handle) { m_poolHandle = handle; }
		PoolHandle& GetPoolHandle() { return m_poolHandle; }

		const ParticleInitializer& GetInitialData() const { return m_initialData; }

		// Frustum Culling용
		Vector3 GetWorldPosition() const;
		float GetBoundingRadius() const { return 1.0f; } // 기본 반경 - 적절한 크기

		// Overdraw Control - Spawn Ratio Update
		void UpdateSpawnRatios(const Vector3& cameraPos);

		// Priority-based eviction getters/setters
		float GetCreationTime() const { return m_creationTime; }
		float GetBasePriority() const { return m_basePriority; }
		void SetBasePriority(float priority) { m_basePriority = std::clamp(priority, 0.0f, 1.0f); }
		void SetCreationTime(float time) { m_creationTime = time; }
	private:
		void Reset();
		void ExecutePreWarm(IndirectArgsBuffer<DispatchArgs>& dispatchArgs);
		void UpdateTransform();

		void RegisterSpawnPositions(ParticleEmitter* emitter, std::vector<Vector3>& outPositions, ParticleConsts& pConsts, EmitterID& eID);

		// SubEmitter
		void OnEmitterEvent(EmitterEvent event, ParticleEmitter* emitter);
		void LoadSubEmitters(ParticleEmitter* emitter,
			ParticleInitializer& initialData, std::set<std::wstring>& processedPaths);
		void ActivateSubEmitter(ParticleEmitter* subEmitter, const Vector3& position);

	private:
		ParticleInitializer m_initialData;
		Actor* m_owner = nullptr;
		bool m_looping = true;
		float m_duration = 5.0f;
		float m_playRate = 1.0f;
		float m_preWarmTime = 0.0f;
		ParticleState m_state = ParticleState::Playing;

		// Priority-based eviction data
		float m_creationTime = 0.0f;    // Timestamp when created
		float m_basePriority = 0.5f;    // User-defined importance (0.0-1.0)

		// Main Emitters
		std::vector<std::unique_ptr<ParticleEmitter>> m_emitters;

		// SubEmitters Emitter 
		std::unordered_map<std::wstring, std::unique_ptr<ParticleEmitter>> m_subEmitterPool;
		//   SubEmitter 
		std::vector<ParticleEmitter*> m_activeSubEmitters;
		std::vector<std::pair<ParticleEmitter*, Vector3>> m_pendingSubEmitters;

		std::wstring m_jsonPath;
		FileWatcher::CallbackID m_watcherID = 0;
		ParticleMeshConsts m_meshConsts;

		// Mesh 
		UINT m_modelIdx = UINT_MAX;
		UINT m_vertexCount = 0;
		UINT m_indexCount = 0;

		PoolHandle m_poolHandle;

		UINT m_currentParticleOffset = 0;
		UINT m_currentEmitterIndex = 0;
		UINT m_maxTotalParticles = 0;
		UINT m_maxEmitters = 0;

		//  SpawnPosition 
		UINT m_currentSpawnPosOffset = 0;
		std::unordered_map<std::string, std::pair<UINT, UINT>> m_spawnPosCache;  // path -> (offset, count)

		IndirectArgsBuffer<DispatchArgs>* m_dispatchArgs = nullptr;
		IndirectArgsBuffer<DrawIndexedInstancedArgs>* m_billboardArgsBuffer = nullptr;
		IndirectArgsBuffer<DrawIndexedInstancedArgs>* m_meshArgsBuffer = nullptr;
	};
}