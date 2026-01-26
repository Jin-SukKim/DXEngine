#include "pch.h"
#include "ParticleSpawner.h"
#include "Scene.h"
#include "TransformComponent.h"
#include <random>

namespace DE {

    ParticleSpawner::ParticleSpawner(const std::wstring& name) : Super(name) {
        AddComponent<TransformComponent>(L"Transform");
    }

    void ParticleSpawner::Initialize() {
        Super::Initialize();
    }

    void ParticleSpawner::Update(const float& deltaTime) {
        Super::Update(deltaTime);

        if (m_stopped) return;

        m_elapsedTime += deltaTime;

        // Spawner 자체 수명 체크
        if (m_autoDestroy && m_lifetime > 0.0f && m_elapsedTime >= m_lifetime) {
            m_stopped = true;
            return;
        }

        // 완료된 Effect 정리 (IsFinished() 체크)
        CleanupExpiredEffects();

        UpdateSpawning(deltaTime);
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

    void ParticleSpawner::CleanupExpiredEffects() {
        if (!m_ownerScene) return;
        
        std::erase_if(m_spawnedEffects, [this](EffectActor* effect) {
            // Scene에 실제로 존재하는지 확인
            return !m_ownerScene->ContainsEffect(effect);
        });
    }

    void ParticleSpawner::Spawn() {
        if (!m_ownerScene) return;
        
        // 현재 활성 개수 확인
        if (static_cast<int>(m_spawnedEffects.size()) >= m_maxActiveParticles) return;

        // 팩토리로 Actor 생성
        auto actor = m_actorFactory(L"SpawnedEffect");
        if (!actor) return;

        // 위치 설정
        Vector3 spawnPos = GetRandomSpawnPosition();
        if (auto* transform = actor->GetComponent<TransformComponent>()) {
            transform->SetPos(spawnPos);
        }
        else {
            auto* newTransform = actor->AddComponent<TransformComponent>(L"Transform");
            newTransform->SetPos(spawnPos);
        }

        // 프리셋 설정 (필요한 경우)
        if (actor->NeedsExternalPreset() && !m_presetPath.empty()) {
            actor->SetParticlePreset(m_presetPath);
        }

        actor->Initialize();

        // raw pointer 저장 (추적용)
        EffectActor* rawPtr = actor.get();
        m_spawnedEffects.push_back(rawPtr);

        // Scene에 추가 (소유권 이전)
        m_ownerScene->SpawnEffect(std::move(actor));
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
            // SpawnBurst 호출로만 동작
            break;
        }
    }

    // 개선된 GetRandomSpawnPosition() - 다양한 형태 지원
    Vector3 ParticleSpawner::GetRandomSpawnPosition() {
        auto* transform = GetComponent<TransformComponent>();
        Vector3 basePos = transform ? transform->GetPos() : Vector3(0.f);

        static std::mt19937 gen{std::random_device{}()};

        switch (m_spawnShape) {
        case SpawnShape::Box: {
            std::uniform_real_distribution<float> distX(-m_spawnBoxExtents.x, m_spawnBoxExtents.x);
            std::uniform_real_distribution<float> distY(-m_spawnBoxExtents.y, m_spawnBoxExtents.y);
            std::uniform_real_distribution<float> distZ(-m_spawnBoxExtents.z, m_spawnBoxExtents.z);
            
            basePos.x += distX(gen);
            basePos.y += distY(gen);
            basePos.z += distZ(gen);
            break;
        }
        case SpawnShape::Sphere: {
            if (m_spawnRadius > 0.0f) {
                std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
                std::uniform_real_distribution<float> radiusDist(0.0f, 1.0f);
                
                float theta = std::acos(dist(gen));
                float phi = dist(gen) * DirectX::XM_PI;
                float r = std::cbrt(radiusDist(gen)) * m_spawnRadius;
                
                basePos.x += r * std::sin(theta) * std::cos(phi);
                basePos.y += r * std::cos(theta);
                basePos.z += r * std::sin(theta) * std::sin(phi);
            }
            break;
        }
        case SpawnShape::Circle:
        default: {
            if (m_spawnRadius > 0.0f) {
                std::uniform_real_distribution<float> angleDist(0.0f, DirectX::XM_2PI);
                std::uniform_real_distribution<float> radiusDist(0.0f, 1.0f);
                
                float angle = angleDist(gen);
                float radius = std::sqrt(radiusDist(gen)) * m_spawnRadius;

                basePos.x += std::cos(angle) * radius;
                basePos.z += std::sin(angle) * radius;
            }
            break;
        }
        }

        return basePos;
    }

}