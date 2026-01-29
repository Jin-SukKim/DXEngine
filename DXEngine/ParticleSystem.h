#pragma once
#include "Object.h"
#include "ParticleEmitter.h"
#include "MeshData.h"

namespace DE {
	struct Mesh;

enum class ParticleState {
	Playing, // 재생
	Paused, // 멈춘 상태
	Stopped // Not Visible인것처럼 처리
};

struct ParticleMeshConsts {
	Matrix world;
	Matrix worldIT; // World Inverse Transpose (Normal 변환에 사용)
	UINT vertexCount;
	UINT indexCount;
	float padding[2];
};

class ParticleSystem : public Object
{
public:
	ParticleSystem(const std::wstring& name);
	~ParticleSystem() override;

	ParticleSystem(const ParticleSystem& other);
	UINT GetEmitterCount() const { return static_cast<UINT>(m_emitters.size()); }

	void Initialize() override;
	void OnSpawn();
	void Update(const float& dt) override;
	void Render() override;

	void AddEmitter(const std::string& path);
	void AddEmitter(std::unique_ptr<ParticleEmitter>&& emitter);
	void ClearEmitters();
	void LoadFromJson(const json& data);
	void SetHotReloadInfo(const std::wstring& path, FileWatcher::CallbackID id);

	// [제어 함수]
	void Play();
	void Pause();
	void Stop();    // 멈추고 파티클 즉시 삭제
	void Restart(); // 초기화 후 다시 재생 (Pre-warm 포함)

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

	// 위치 오프셋 설정 (모든 Emitter에 적용)
	void SetSpawnOffset(const Vector3& offset);

	// 모든 Emitter 종료 확인 (SubEmitter 포함)
	bool IsAllEmittersCompleted() const;

	StructuredBuffer<Particle>& GetReadBuffer() { return m_particles[m_currentBuffer]; }
	StructuredBuffer<Particle>& GetWriteBuffer() { return m_particles[1 - m_currentBuffer]; }
	StructuredBuffer<uint32_t>& GetReadCount() { return m_activeCounts[m_currentBuffer]; }
	StructuredBuffer<uint32_t>& GetWriteCount() { return m_activeCounts[1 - m_currentBuffer]; }
	
	IndirectArgsBuffer<DispatchArgs>& GetDispatchArgs() { return m_dispatchArgs; }
	UINT GetDispatchArgsOffset(UINT emitterID) { return emitterID * 12; }
	
	IndirectArgsBuffer<DrawInstancedArgs>& GetBillboardArgs() { return m_billboardArgsBuffer; }
	UINT GetBillboardArgsOffset(UINT emitterID) { return emitterID * 16; }
	
	IndirectArgsBuffer<DrawIndexedInstancedArgs>& GetMeshArgs() { return m_meshArgsBuffer; }
	UINT GetMeshArgsOffset(UINT emitterID) { return emitterID * 20; }
	DrawIndexedInstancedArgs& GetInitMeshArgs(UINT emitterID) { return m_initMeshArgs[emitterID]; }
	
	StructuredBuffer<ParticleConsts>& GetConstsBuffer() { return m_consts; }
	ParticleConsts& GetConstsData(UINT emitterID) { return m_consts.Get(emitterID); }
	
	StructuredBuffer<ParticleFrameConsts>& GetFrameConstsBuffer() { return m_frameConsts; }
	ParticleFrameConsts& GetFrameConstsData(UINT emitterID) { return m_frameConsts.Get(emitterID); }
	
	void BindConstantID(UINT emitterID);
	StructuredBuffer<Vector3>& GetBakedSpawnBuffer() { return m_bakedSpawnPos; }

	void SwapBuffer() { m_currentBuffer = 1 - m_currentBuffer; }
private:
	void Reset();
	void ExecutePreWarm();
	void UpdateTransform();

	void RegisterEmitter(ParticleEmitter* emitter, uint32_t capacity);
	void RegisterBakedPos(ParticleEmitter* emitter);

	// SubEmitter 처리
	void OnEmitterEvent(EmitterEvent event, ParticleEmitter* emitter);
	void SpawnSubEmitter(const SubEmitter& sub, const Vector3& position);
private:
	Actor* m_owner = nullptr;
	bool m_looping = true;
	float m_duration = 5.0f;
	float m_playRate = 1.0f;
	float m_preWarmTime = 0.0f; // 시작 시 미리 시뮬레이션 돌릴 시간 (예: 안개)
	ParticleState m_state = ParticleState::Playing;

	std::vector<std::unique_ptr<ParticleEmitter>> m_emitters;
	std::wstring m_jsonPath;
	FileWatcher::CallbackID m_watcherID = 0;
	ConstantBuffer<ParticleMeshConsts> m_meshConsts;

	// Mesh 데이터 (Vertex/Surface Spawn용)
	StructuredBuffer<Vector3> m_meshVertex;
	StructuredBuffer<uint32_t> m_meshIndices;
	UINT m_vertexCount = 0;
	UINT m_indexCount = 0;

	// 동적으로 생성된 Sub-Emitter
	std::vector<std::unique_ptr<ParticleEmitter>> m_dynamicEmitters;

	// 파티클 버퍼 (이중 버퍼링)
	StructuredBuffer<uint32_t> m_activeCounts[2];
	StructuredBuffer<Particle> m_particles[2];
	UINT m_currentBuffer = 0;
	UINT m_currentParticleOffset = 0; // 다음 emitter에게 할당할 particle 시작 위치
	UINT m_currentEmitterIndex = 0; // 다음 emitter에게 할당할 ID
	UINT m_maxTotalParticles = 1000000; // 최대 Particle 개수
	UINT m_maxEmitters = 64; // 최대 emitter 개수

	IndirectArgsBuffer<DispatchArgs> m_dispatchArgs;
	StructuredBuffer<ParticleConsts> m_consts;
	StructuredBuffer<ParticleFrameConsts> m_frameConsts;
	std::vector<ConstantBuffer<EmitterID>> m_emitterIDs;

	StructuredBuffer<Vector3> m_bakedSpawnPos;
	UINT m_currentBakedOffset = 0;
	std::unordered_map<std::string, std::pair<UINT, UINT>> m_bakedOffset;

	IndirectArgsBuffer<DrawInstancedArgs> m_billboardArgsBuffer;
	IndirectArgsBuffer<DrawIndexedInstancedArgs> m_meshArgsBuffer;
	std::vector<DrawIndexedInstancedArgs> m_initMeshArgs;

};

}
