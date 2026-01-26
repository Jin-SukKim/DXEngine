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
    CleanupDeadSystems();
}

void ParticleSpawner::Render() {
    Super::Render();
}

void ParticleSpawner::SetParticlePreset(const std::wstring& presetPath) {
    m_presetPath = L"..\\Assets\\" + presetPath;
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
    if (m_spawnedSystems.size() >= m_maxActiveParticles) return;

    // ParticleManager를 통해 시스템 인스턴스 생성
    ParticleSystem* system = ParticleManager::Get().CreateSystem(m_presetPath);
    if (system) {
        // Transform 설정 (Spawner 위치 + 랜덤 반경)
        auto* transform = GetComponent<TransformComponent>();
        if (transform) {
            Vector3 spawnPos = GetRandomSpawnPosition();

            MeshConstants meshConst;
            meshConst.world = Matrix::CreateTranslation(spawnPos).Transpose();
            system->SetTransform(meshConst);
        }

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
            // Burst 모드는 자동 생성을 하지 않음 (SpawnBurst 호출 시에만 동작)
            break;
    }
}

void ParticleSpawner::CleanupDeadSystems() {
    auto it = std::remove_if(m_spawnedSystems.begin(), m_spawnedSystems.end(),
        [](ParticleSystem* system) {
            if (system == nullptr) return true;

            // 파티클이 멈췄다면 (Loop: false이고 재생 끝남)
            if (system->IsStopped()) {
                // [수정] 직접 delete 하지 않고 Manager에 파괴 요청
                ParticleManager::Get().DestroyInstance(system);

                return true; // Spawner의 관리 목록에서도 제거
            }

            return false;
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