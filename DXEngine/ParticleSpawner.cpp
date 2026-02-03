#include "pch.h"
#include "ParticleSpawner.h"
#include "ParticleManager.h"
#include "TransformComponent.h"
#include "Scene.h"

namespace DE {

    ParticleSpawner::ParticleSpawner(const std::wstring& name) : Super(name) {
        // Spawner 위치 지정을 위해 Transform 추가
        AddComponent<TransformComponent>(L"Transform");
    }

    ParticleSpawner::~ParticleSpawner() {}

    void ParticleSpawner::Initialize() {
        Super::Initialize();
    }

    void ParticleSpawner::Update(const float& deltaTime) {
        Super::Update(deltaTime);

        if (m_stopped)
            return;

        m_elapsedTime += deltaTime;

        // Spawner 자체 수명 관리
        if (m_autoDestroy && m_lifetime > 0.0f && m_elapsedTime >= m_lifetime) {
            m_stopped = true;
            return;
        }

        // 종료된 Effect 정리
        CleanupDeadSystems();
        // 새로 Spawn
        UpdateSpawning(deltaTime);
    }

    void ParticleSpawner::Render() {
        Super::Render();
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

    void ParticleSpawner::SetSpawnBox(Vector3 halfExtends)
    {
        m_spawnBoxExtents = halfExtends;
    }

    void ParticleSpawner::SetAutoDestroy(bool enable) {
        m_autoDestroy = enable;
    }

    void ParticleSpawner::SetLifetime(float lifetime) {
        m_lifetime = lifetime;
    }

    // Spawn 함수 구현
    void ParticleSpawner::Spawn() {
        if (!m_scene)
            return;

        // 최대 개수 제한 확인
        if (m_spawnedEffects.size() >= m_maxActiveParticles) return;

        // 팩토리를 통해 Actor 생성 (EffectActor 또는 EffectActor를 상속받은 Class 등)
        auto actor = m_actorFactory(L"SpawnedEffect");
        if (!actor) return;
        if (actor->FailedToCreate()) 
            return;
        

        // 위치 설정 (Spawner 위치 기준 랜덤 반경)
        Vector3 spawnPos = GetRandomSpawnPosition();
        if (auto* tr = actor->GetComponent<TransformComponent>())
            tr->SetPos(spawnPos);

        // 프리셋 설정 여부 확인
        if (actor->NeedsExternalPreset() && !m_presetPath.empty()) 
            actor->SetParticlePreset(m_presetPath);

        // 초기화 
        actor->Initialize();

        // raw pointer 저장 (추적용)
        EffectActor* rawPtr = actor.get();
        m_spawnedEffects.push_back(rawPtr);

        // Scene에 추가
        m_scene->SpawnEffect(std::move(actor));
    }

    void ParticleSpawner::SpawnBurst(int count) {
        for (int i = 0; i < count; ++i) {
            Spawn();
        }
    }

    void ParticleSpawner::Stop() {
        m_stopped = true;
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
            if (m_spawnedEffects.empty()) {
                Spawn();
            }
            break;

        case SpawnMode::Burst:
            // Burst 모드는 자동 생성 안 함 (SpawnBurst 호출 시 작동)
            break;
        }
    }

    void ParticleSpawner::CleanupDeadSystems() {
        if (!m_scene) return;

        std::erase_if(m_spawnedEffects, [this](EffectActor* effect) {
            // Scene에 실제로 존재하는지 확인
            return !m_scene->ContainsEffect(effect) || effect->FailedToCreate();
            });
    }

    Vector3 ParticleSpawner::GetRandomSpawnPosition() {
        auto* transform = GetComponent<TransformComponent>();
        Vector3 basePos = transform ? transform->GetPos() : Vector3(0.f);

        static std::mt19937 gen{ std::random_device{}() };

        std::uniform_real_distribution<float> distX(-m_spawnBoxExtents.x, m_spawnBoxExtents.x);
        std::uniform_real_distribution<float> distY(-m_spawnBoxExtents.y, m_spawnBoxExtents.y);
        std::uniform_real_distribution<float> distZ(-m_spawnBoxExtents.z, m_spawnBoxExtents.z);

        basePos.x += distX(gen);
        basePos.y += distY(gen);
        basePos.z += distZ(gen);

        return basePos;
    }

}