#include "pch.h"
#include "ParticleSpawner.h"
#include "ParticleManager.h"
#include "TransformComponent.h"

namespace DE {

ParticleSpawner::ParticleSpawner(const std::wstring& name) : Super(name) {
    AddComponent<TransformComponent>(L"Transform");
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

    // Lifetime 체크
    if (m_autoDestroy && m_lifetime > 0.0f && m_elapsedTime >= m_lifetime) {
        Clear();
        return;
    }

    UpdateSpawning(deltaTime);
    CleanupDeadSystems();
}

void ParticleSpawner::Render() {
    Super::Render();
}

void ParticleSpawner::SetParticlePreset(const std::wstring& presetPath) {
    m_presetPath = presetPath;
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
    if (m_spawnedSystems.size() >= m_maxActiveParticles) return;

    // ParticleManager를 통해 생성
    ParticleSystem* system = ParticleManager::Get().CreateSystem(m_presetPath);
    if (system) {
        // Transform 설정
        auto* transform = GetComponent<TransformComponent>();
        if (transform) {
            Vector3 spawnPos = GetRandomSpawnPosition();
            
            MeshConstants meshConst;
            meshConst.world = Matrix::CreateTranslation(spawnPos).Transpose();
            system->SetTransform(meshConst);
        }

        system->Initialize();
        system->OnSpawn();
        system->Play();

        m_spawnedSystems.push_back(system);
    }
}

void ParticleSpawner::SpawnBurst(int count) {
    for (int i = 0; i < count; ++i) {
        Spawn();
    }
}

void ParticleSpawner::Stop() {
    for (auto* system : m_spawnedSystems) {
        if (system) {
            system->Stop();
        }
    }
}

void ParticleSpawner::Clear() {
    m_spawnedSystems.clear();
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
            if (m_spawnedSystems.empty()) {
                Spawn();
            }
            break;

        case SpawnMode::Burst:
            // 수동 제어
            break;
    }
}

void ParticleSpawner::CleanupDeadSystems() {
    auto it = std::remove_if(m_spawnedSystems.begin(), m_spawnedSystems.end(),
        [](ParticleSystem* system) {
            // ParticleSystem이 종료되었는지 확인
            // (ParticleSystem에 IsAlive() 메서드 필요)
            return system == nullptr;
        });
    
    m_spawnedSystems.erase(it, m_spawnedSystems.end());
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