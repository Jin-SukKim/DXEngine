#pragma once
#include "Object.h"
#include "ParticleEmitter.h"
#include "MeshData.h"

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

	void Play();
	void Pause();
	void Stop();
	void Restart();

	void SetLooping(bool loop) { m_looping = loop; }
	void SetDuration(float duration) { m_duration = duration; }
	void SetPlayRate(float rate) { m_playRate = rate; }
	void SetPreWarmTime(float time) { m_preWarmTime = time; }
	void SetTargetMesh(const int& modelIdx);

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

	// 葛电 Emitter 肯丰 咯何
	bool IsAllEmittersCompleted() const;
	
private:
	void Reset();
	void ExecutePreWarm();
	void UpdateTransform();
	
	// Sub-Emitter 贸府
	void OnEmitterEvent(EmitterEvent event, ParticleEmitter* emitter);
	void SpawnSubEmitter(const SubEmitterEntry& entry, const Vector3& position);

private:
	Actor* m_owner = nullptr;
	bool m_looping = true;
	float m_duration = 5.0f;
	float m_playRate = 1.0f;
	float m_time = 0.f;
	float m_preWarmTime = 0.0f;
	ParticleState m_state = ParticleState::Playing;

	std::vector<std::unique_ptr<ParticleEmitter>> m_emitters;
	std::wstring m_jsonPath;
	FileWatcher::CallbackID m_watcherID = 0;
	ConstantBuffer<ParticleMeshConsts> m_meshConsts;

	StructuredBuffer<Vector3> m_meshVertex;
	StructuredBuffer<uint32_t> m_meshIndices;
	UINT m_vertexCount = 0;
	UINT m_indexCount = 0;
	
	// 悼利栏肺 积己等 Sub-Emitter
	std::vector<std::unique_ptr<ParticleEmitter>> m_dynamicEmitters;
};

}
