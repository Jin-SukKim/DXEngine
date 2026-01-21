#pragma once
#include "Component.h"
#include "ParticleEmitter.h"
#include "MeshData.h"

namespace DE {

enum class ParticleState {
	Playing, // 재생
	Paused, // 멈춘 상태
	Stopped // Not Visible인것처럼 처리
};

class ParticleSystem : public Component
{
public:
	ParticleSystem(const std::wstring& name);
	~ParticleSystem() override;

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
	void SetTargetMesh(const MeshData& meshe);
private:
	void Reset();
	void ExecutePreWarm();

private:
	bool m_looping = true;
	float m_duration = 5.0f;
	float m_playRate = 1.0f;
	float m_time = 0.f;
	float m_preWarmTime = 0.0f; // 시작 시 미리 시뮬레이션 돌릴 시간 (예: 안개)
	ParticleState m_state = ParticleState::Playing;

	std::vector<std::unique_ptr<ParticleEmitter>> m_emitters;
	std::wstring m_jsonPath;
	FileWatcher::CallbackID m_watcherID = 0;
};

}
