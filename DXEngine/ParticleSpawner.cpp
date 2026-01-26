#include "pch.h"
#include "ParticleSpawner.h"
#include "ParticleManager.h"
#include "TransformComponent.h"

namespace DE {

    ParticleSpawner::ParticleSpawner(const std::wstring& name) : Super(name) {
        // Spawner 위치 지정을 위해 Transform 추가
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

        // Spawner 자체 수명 관리
        if (m_autoDestroy && m_lifetime > 0.0f && m_elapsedTime >= m_lifetime) {
            Clear();
            // 필요하다면 여기서 Spawner Actor 자체를 Scene에서 제거 요청 (Destroy(this))
            return;
        }

        UpdateSpawning(deltaTime);

        // 관리 중인 모든 이펙트 Actor 업데이트
        for (auto& actor : m_spawnedActors) {
            if (actor) actor->Update(deltaTime);
        }

        CleanupDeadSystems();
    }

    void ParticleSpawner::Render() {
        Super::Render();
        for (auto& actor : m_spawnedActors) {
            if (actor) actor->Render();
        }
    }

    void ParticleSpawner::SetParticlePreset(const std::wstring& presetPath) {
        // 경로 설정 (접두어 처리 등은 프로젝트 규칙에 따름)
        m_presetPath = presetPath;
        // 예: m_presetPath = L"..\\Assets\\" + presetPath;
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

    // [핵심 수정] Spawn 함수 구현
    void ParticleSpawner::Spawn() {
        // 1. 최대 개수 제한 확인
        if (m_spawnedActors.size() >= m_maxActiveParticles) return;

        // 2. 팩토리를 통해 Actor 생성 (EffectActor 또는 Firework 등)
        std::unique_ptr<EffectActor> actor = m_actorFactory(L"SpawnedEffect");
        if (!actor) return;

        // 3. 위치 설정 (Spawner 위치 기준 랜덤 반경)
        auto* transform = actor->GetComponent<TransformComponent>();
        if (transform) {
            transform->SetPos(GetRandomSpawnPosition());
        }

        // 4. 프리셋 설정 여부 확인 (다형성 활용)
        // Firework 등 내부 생성 방식은 NeedsExternalPreset() == false 이므로 패스
        if (actor->NeedsExternalPreset()) {
            // 외부 프리셋이 필요한데 경로가 비어있으면 생성 취소
            if (m_presetPath.empty()) {
                return; // unique_ptr이 범위를 벗어나며 자동 소멸됨
            }
            actor->SetParticlePreset(m_presetPath);
        }

        // 5. 초기화 및 리스트 추가
        actor->Initialize();
        m_spawnedActors.push_back(std::move(actor));
    }

    void ParticleSpawner::SpawnBurst(int count) {
        for (int i = 0; i < count; ++i) {
            Spawn();
        }
    }

    void ParticleSpawner::Stop() {
        for (auto& actor : m_spawnedActors) {
            if (actor) actor->Stop();
        }
    }

    void ParticleSpawner::Clear() {
        m_spawnedActors.clear(); // unique_ptr 벡터이므로 clear 시 자동 소멸(delete)
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
            // Burst 모드는 자동 생성 안 함 (SpawnBurst 호출 시 작동)
            break;
        }
    }

    void ParticleSpawner::CleanupDeadSystems() {
        // 종료된 이펙트 제거 (IsFinished()가 true인 요소 삭제)
        auto it = std::remove_if(m_spawnedActors.begin(), m_spawnedActors.end(),
            [](const std::unique_ptr<EffectActor>& actor) {
                if (!actor) return true;
                return actor->IsFinished();
            });

        m_spawnedActors.erase(it, m_spawnedActors.end());
    }

    Vector3 ParticleSpawner::GetRandomSpawnPosition() {
        auto* transform = GetComponent<TransformComponent>();
        Vector3 basePos = transform ? transform->GetPos() : Vector3(0.f);

        if (m_spawnRadius > 0.0f) {
            // XZ 평면상의 랜덤 위치 계산
            float angle = (float)(rand() % 360) * 3.14159f / 180.0f;
            float radius = ((float)(rand() % 100) / 100.0f) * m_spawnRadius;

            basePos.x += cos(angle) * radius;
            basePos.z += sin(angle) * radius;
        }

        return basePos;
    }

}