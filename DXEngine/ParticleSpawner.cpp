#include "pch.h"
#include "ParticleSpawner.h"
#include "ParticleManager.h"
#include "TransformComponent.h"

namespace DE {

ParticleSpawner::ParticleSpawner(const std::wstring& name) : Super(name) {
}

ParticleSpawner::~ParticleSpawner() {
    Clear();
}

void ParticleSpawner::Initialize() {
    Super::Initialize();
}

void ParticleSpawner::Update(const float& deltaTime) {
    Super::Update(deltaTime);

    m_elapsedTime += deltaTime;

    // Spawner 자체의 수명 체크 (AutoDestroy가 켜져있을 경우)
    if (m_autoDestroy && m_lifetime > 0.0f && m_elapsedTime >= m_lifetime) {
        Clear();
        return;
    }

    UpdateSpawning(deltaTime);

    for (auto& actor : m_spawnedActors) {
        actor->Update(deltaTime);
    }

    CleanupDeadSystems();
}

void ParticleSpawner::Render() {
    Super::Render();

    for (auto& actor : m_spawnedActors) {
        actor->Render();
    }
}

void ParticleSpawner::SetParticlePreset(const std::vector<std::wstring>& presetPath) {
    for (const std::wstring& path : presetPath)
        m_presetPath.push_back(L"..\\Assets\\" + path);
}

void ParticleSpawner::SetSpawnMode(SpawnMode mode) {
    m_spawnMode = mode;
}

void ParticleSpawner::SetSpawnInterval(float interval) {
    m_spawnInterval = interval;
}

void ParticleSpawner::SetMaxActiveParticles(int maxCount) {
    m_maxActiveParticles = maxCount;
}

void ParticleSpawner::SetSpawnRadius(float radius) {
    m_spawnRadius = radius;
}

void ParticleSpawner::SetAutoDestroy(bool enable) {
    m_autoDestroy = enable;
}

void ParticleSpawner::SetLifetime(float lifetime) {
    m_lifetime = lifetime;
}

void ParticleSpawner::Spawn() {
    if (m_presetPath.empty()) return;
    // 최대 활성 개수 제한 확인
    if (m_spawnedActors.size() >= m_maxActiveParticles) return;

    // 1. EffectActor 생성
    std::unique_ptr<EffectActor> actor = std::make_unique<EffectActor>(L"SpawnedEffect");

    // 2. 위치 설정 (Spawner 위치 기준 랜덤)
    auto* transform = actor->GetComponent<TransformComponent>();
    if (transform) {
        transform->SetPos(GetRandomSpawnPosition());
    }

    // 3. 파티클 프리셋 지정
    actor->SetParticlePreset(m_presetPath);

    // 4. 초기화 및 리스트 추가
    actor->Initialize();
    m_spawnedActors.push_back(std::move(actor));
}

void ParticleSpawner::SpawnBurst(int count) {
    for (int i = 0; i < count; ++i) {
        Spawn();
    }
}

void ParticleSpawner::Stop() {
    for (auto& system : m_spawnedActors) {
        if (system) {
            system->Stop();
        }
    }
}

void ParticleSpawner::Clear() {
    m_spawnedActors.clear();
    // ParticleManager가 자동으로 정리
}

void ParticleSpawner::UpdateSpawning(float dt) {
    switch (m_spawnMode) {
        case SpawnMode::Continuous:
            Spawn();
            break;

        case SpawnMode::Interval:
            m_spawnAccumulator += dt;
            if (m_spawnAccumulator >= m_spawnInterval) {
                Spawn();
                m_spawnAccumulator -= m_spawnInterval;
            }
            break;

        case SpawnMode::OneShot:
            if (m_spawnedActors.empty()) {
                Spawn();
            }
            break;

        case SpawnMode::Burst:
            // Burst 모드는 자동 생성을 하지 않음 (SpawnBurst 호출 시에만 동작)
            break;
    }
}

void ParticleSpawner::CleanupDeadSystems() {
    auto it = std::remove_if(m_spawnedActors.begin(), m_spawnedActors.end(),
        [](std::unique_ptr<EffectActor>& actor) {
            if (!actor) return true;

            // 파티클이 멈췄는지 확인
            std::vector<ParticleSystem*> paritcles = actor->GetParticleSystem();
            for (auto& ps : paritcles)
                if (ps && ps->IsPlaying())
                    return false;
            return true;
        });

    m_spawnedActors.erase(it, m_spawnedActors.end());
}

Vector3 ParticleSpawner::GetRandomSpawnPosition() {
    auto* transform = GetComponent<TransformComponent>();
    Vector3 basePos = transform ? transform->GetPos() : Vector3(0.f);

    if (m_spawnRadius > 0.0f) {
        // 랜덤 원형 분포
        float angle = (float)(rand() % 360) * 3.14159f / 180.0f;
        float radius = ((float)(rand() % 100) / 100.0f) * m_spawnRadius;
        
        basePos.x += cos(angle) * radius;
        basePos.z += sin(angle) * radius;
    }

    return basePos;
}

}