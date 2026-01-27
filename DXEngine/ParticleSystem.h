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

	// 모든 Emitter 종료 확인 (SubEmitter 포함)
	bool IsAllEmittersCompleted() const;
private:
	void Reset();
	void ExecutePreWarm();
	void UpdateTransform();

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
};

}
