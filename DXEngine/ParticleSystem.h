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

	struct ParticleMeshConsts {
		Matrix world;
		Matrix worldIT;
		UINT vertexCount;
		UINT indexCount;
		float padding[2];
	};

	struct ParticleInitializer {
		std::vector<ParticleConsts> consts;
		std::vector<ParticleFrameConsts> frameConsts;
		std::vector<DrawIndexedInstancedArgs> initMeshArgs;
		std::vector<Vector3> bakedPositions;
		std::vector<Vector3> customPositions;
		std::vector<EmitterID> emitterIDs;
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
		void InitializeGPU(ParticleInitializer& initialData);

		void OnSpawn();
		void PreUpdate(const float& dt, std::vector<ParticleFrameConsts>& fsConsts);
		void Update(const float& dt) override;
		void ActivateSubEmitters();
		void UpdateArgs(Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context);
		void Render() override;

		void AddEmitter(const std::string& path);
		void AddEmitter(std::unique_ptr<ParticleEmitter>&& emitter);
		void ClearEmitters();
		void LoadFromJson(const json& data);
		void SetHotReloadInfo(const std::wstring& path, FileWatcher::CallbackID id);
		void ProcessEmitter(
			ParticleEmitter* emitter,
			ParticleInitializer& initialData);

		// [제어 함수]
		void Play();
		void Pause();
		void Stop();
		void Restart();

		// [속성 설정]
		void SetLooping(bool loop) { m_looping = loop; }
		void SetDuration(float duration) { m_duration = duration; }
		void SetPlayRate(float rate) { m_playRate = rate; }
		void SetPreWarmTime(float time) { m_preWarmTime = time; }
		void SetTargetMesh(const int& modelIdx);

		// Mesh 데이터 접근자
		StructuredBuffer<Vector3>* GetMeshVertexBuffer() { return &m_meshVertex; }
		StructuredBuffer<uint32_t>* GetMeshIndexBuffer() { return &m_meshIndices; }
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

		IndirectArgsBuffer<DispatchArgs>& GetDispatchArgs() { return m_dispatchArgs; }
		UINT GetDispatchArgsOffset(UINT emitterID) { return emitterID * 12; }

		IndirectArgsBuffer<DrawInstancedArgs>& GetBillboardArgs() { return m_billboardArgsBuffer; }
		UINT GetBillboardArgsOffset(UINT emitterID) { return emitterID * 16; }

		IndirectArgsBuffer<DrawIndexedInstancedArgs>& GetMeshArgs() { return m_meshArgsBuffer; }
		UINT GetMeshArgsOffset(UINT emitterID) { return emitterID * 20; }

		void BindConstantID(UINT emitterID);
		StructuredBuffer<Vector3>& GetBakedSpawnBuffer() { return m_bakedSpawnPos; }
		StructuredBuffer<Vector3>& GetCustomPositions() { return m_customPositions; }

		void SwapBuffer() { m_currentBuffer = 1 - m_currentBuffer; }

		UINT GetTotalParticleCount() const { return m_maxTotalParticles; }
		UINT GetMaxEmitterCount() const { return m_maxEmitters; }
		void SetPoolHandle(PoolHandle handle) { m_poolHandle = handle; }
		PoolHandle GetPoolHandle() const { return m_poolHandle; }
	private:
		void Reset();
		void ExecutePreWarm();
		void UpdateTransform();

		void RegisterEmitter(ParticleEmitter* emitter, uint32_t capacity, EmitterID& eID);
		void RegisterBakedPos(ParticleEmitter* emitter, std::vector<Vector3>& positions, ParticleConsts& pConsts, EmitterID& eID);
		void RegisterCustomPos(ParticleEmitter* emitter, std::vector<Vector3>& positions, EmitterID& eID);

		// SubEmitter 처리 (단순화)
		void OnEmitterEvent(EmitterEvent event, ParticleEmitter* emitter);
		void LoadSubEmitters(ParticleEmitter* emitter,
			ParticleInitializer& initialData);
		void ActivateSubEmitter(ParticleEmitter* subEmitter, const Vector3& position);

	private:
		Actor* m_owner = nullptr;
		bool m_looping = true;
		float m_duration = 5.0f;
		float m_playRate = 1.0f;
		float m_preWarmTime = 0.0f;
		ParticleState m_state = ParticleState::Playing;

		// Main Emitters
		std::vector<std::unique_ptr<ParticleEmitter>> m_emitters;

		// SubEmitters (미리 로드됨, 경로 -> Emitter 매핑)
		std::unordered_map<std::wstring, std::unique_ptr<ParticleEmitter>> m_subEmitterPool;
		// 현재 활성화된 SubEmitter 포인터들
		std::vector<ParticleEmitter*> m_activeSubEmitters;
		std::vector<std::pair<ParticleEmitter*, Vector3>> m_pendingSubEmitters;

		std::wstring m_jsonPath;
		FileWatcher::CallbackID m_watcherID = 0;
		ConstantBuffer<ParticleMeshConsts> m_meshConsts;

		// Mesh 데이터
		StructuredBuffer<Vector3> m_meshVertex;
		StructuredBuffer<uint32_t> m_meshIndices;
		UINT m_vertexCount = 0;
		UINT m_indexCount = 0;

		PoolHandle m_poolHandle;

		// Defragmentation때 따로 활용
		UINT m_particleReadOffset = 0;
		UINT m_particleWriteOffset = 0;

		// 파티클 버퍼 (이중 버퍼링)
		UINT m_currentBuffer = 0;
		UINT m_currentParticleOffset = 0;
		UINT m_currentEmitterIndex = 0;
		UINT m_maxTotalParticles = 0;
		UINT m_maxEmitters = 0;

		IndirectArgsBuffer<DispatchArgs> m_dispatchArgs;
		std::vector<ConstantBuffer<EmitterID>> m_emitterIDs;

		StructuredBuffer<Vector3> m_bakedSpawnPos;
		UINT m_currentBakedOffset = 0;
		std::unordered_map<std::string, std::pair<UINT, UINT>> m_bakedOffset;
		StructuredBuffer<Vector3> m_customPositions;
		UINT m_currentCustomOffset = 0;

		IndirectArgsBuffer<DrawInstancedArgs> m_billboardArgsBuffer;
		IndirectArgsBuffer<DrawIndexedInstancedArgs> m_meshArgsBuffer;
	};
}