#pragma once
#include "Actor.h"
#include "ParticleSystem.h"

namespace DE {

enum class SpawnMode {
    Continuous,     // 지속적으로 생성 (매 프레임)
    Interval,       // 일정 간격으로 생성
    OneShot,        // 한 번만 생성
    Burst           // 짧은 시간에 대량 생성 (수동 호출)
};

class ParticleSpawner : public Actor {
    using Super = Actor;
public:
    ParticleSpawner(const std::wstring& name);
    virtual ~ParticleSpawner() override;

    void Initialize() override;
    void Update(const float& deltaTime) override;
    void Render() override;

    // [Spawner 설정]
    void SetParticlePreset(const std::wstring& presetPath);
    void SetSpawnMode(SpawnMode mode);
    void SetSpawnInterval(float interval);
    void SetMaxActiveParticles(int maxCount);
    void SetSpawnRadius(float radius);
    void SetAutoDestroy(bool enable);
    void SetLifetime(float lifetime);

    // [수동 제어]
    void Spawn();                   // 단일 파티클 시스템 생성
    void SpawnBurst(int count);     // 한 번에 여러 개 생성
    void Stop();                    // 관리 중인 모든 시스템 정지
    void Clear();                   // 관리 목록 비우기

private:
    void UpdateSpawning(float dt);
    void CleanupDeadSystems();
    Vector3 GetRandomSpawnPosition();

private:
    std::wstring m_presetPath;
    SpawnMode m_spawnMode = SpawnMode::Continuous;

    // 생성 제어 변수
    float m_spawnInterval = 1.0f;
    float m_spawnAccumulator = 0.0f;
    int m_maxActiveParticles = 10;
    float m_spawnRadius = 0.0f;

    // Spawner 수명 관리
    bool m_autoDestroy = false;
    float m_lifetime = -1.0f;
    float m_elapsedTime = 0.0f;

    // 이 Spawner가 생성한 ParticleSystem 포인터 관리
    std::vector<ParticleSystem*> m_spawnedSystems;
};

}