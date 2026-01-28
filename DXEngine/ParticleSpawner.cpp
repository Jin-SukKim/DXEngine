#include "pch.h"
#include "ParticleSpawner.h"
#include "ParticleManager.h"
#include "TransformComponent.h"
#include "Scene.h"

namespace DE {

    ParticleSpawner::ParticleSpawner(const std::wstring& name) : Super(name) {
        // Spawner ��ġ ������ ���� Transform �߰�
        AddComponent<TransformComponent>(L"Transform");
    }

    ParticleSpawner::~ParticleSpawner() {}

    void ParticleSpawner::Initialize() {
        Super::Initialize();
        InitializePool();
    }

    void ParticleSpawner::Update(const float& deltaTime) {
        Super::Update(deltaTime);

        if (m_stopped)
            return;

        m_elapsedTime += deltaTime;

        // Spawner ��ü ���� ����
        if (m_autoDestroy && m_lifetime > 0.0f && m_elapsedTime >= m_lifetime) {
            m_stopped = true;
            return;
        }

        // ����� Effect ����
        CleanupDeadSystems();
        // ���� Spawn
        UpdateSpawning(deltaTime);
    }

    void ParticleSpawner::Render() {
        Super::Render();
    }

    void ParticleSpawner::SetParticlePreset(const std::wstring& presetPath) {
        // ��� ���� (���ξ� ó�� ���� ������Ʈ ��Ģ�� ����)
        m_presetPath = presetPath;
        // ��: m_presetPath = L"..\\Assets\\" + presetPath;
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

    void ParticleSpawner::SetPoolSize(int size) {
        m_poolSize = size;
    }

    // Spawn 함수 구현
    void ParticleSpawner::Spawn() {
        if (!m_scene)
            return;

        // 최대 활성 개수 확인
        if (m_activeEffects.size() >= m_maxActiveParticles) return;

        // Pool에서 가져오기
        EffectActor* actor = AcquireFromPool();
        if (!actor) return;

        // 위치 설정 (Spawner 위치 기준 랜덤 반경)
        Vector3 spawnPos = GetRandomSpawnPosition();
        if (auto* tr = actor->GetComponent<TransformComponent>())
            tr->SetPos(spawnPos);

        // 프리셋 재설정 (필요시)
        if (actor->NeedsExternalPreset() && !m_presetPath.empty()) {
            actor->SetParticlePreset(m_presetPath);
        }

        // ParticleSystem 재시작
        if (auto* ps = actor->GetParticleSystem()) {
            ps->Restart();
            ps->Play();
        }

        // 활성 리스트에 추가
        m_activeEffects.push_back(actor);
        m_spawnedEffects.push_back(actor);  // 호환성 유지
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
            // Burst ���� �ڵ� ���� �� �� (SpawnBurst ȣ�� �� �۵�)
            break;
        }
    }

    void ParticleSpawner::CleanupDeadSystems() {
        if (!m_scene) return;

        // 완료된 Effect를 Pool로 반환
        std::erase_if(m_activeEffects, [this](EffectActor* effect) {
            if (effect->IsFinished()) {
                ReleaseToPool(effect);
                return true;
            }
            return false;
        });

        // m_spawnedEffects도 동기화
        std::erase_if(m_spawnedEffects, [this](EffectActor* effect) {
            return effect->IsFinished();
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

    // ObjectPool 관련 메서드 구현
    void ParticleSpawner::InitializePool() {
        // Pool 초기화: m_poolSize만큼 EffectActor를 미리 생성
        m_pool.clear();
        m_pool.reserve(m_poolSize);

        for (int i = 0; i < m_poolSize; ++i) {
            auto actor = m_actorFactory(L"PooledEffect_" + std::to_wstring(i));
            if (actor) {
                actor->Initialize();
                m_pool.push_back(std::move(actor));
            }
        }
    }

    EffectActor* ParticleSpawner::AcquireFromPool() {
        // Pool에서 사용 가능한 Actor 가져오기
        if (m_pool.empty()) {
            // Pool이 비어있으면 새로 생성 (동적 확장)
            auto actor = m_actorFactory(L"DynamicEffect");
            if (!actor) return nullptr;

            actor->Initialize();
            EffectActor* rawPtr = actor.get();

            // Scene에 추가
            if (m_scene) {
                m_scene->SpawnEffect(std::move(actor));
            }

            return rawPtr;
        }

        // Pool에서 꺼내기
        auto actor = std::move(m_pool.back());
        m_pool.pop_back();

        EffectActor* rawPtr = actor.get();

        // Scene에 추가
        if (m_scene) {
            m_scene->SpawnEffect(std::move(actor));
        }

        return rawPtr;
    }

    void ParticleSpawner::ReleaseToPool(EffectActor* effect) {
        if (!effect || !m_scene) return;

        // Scene에서 소유권 회수
        auto& effectList = m_scene->GetActorList(Scene::ActorCategory::Effect);
        
        for (auto it = effectList.begin(); it != effectList.end(); ++it) {
            if (it->get() == effect) {
                // 소유권을 다시 가져와서 Pool에 저장
                auto actor = std::move(*it);
                effectList.erase(it);

                // Effect를 정지 상태로 만들기
                effect->Stop();

                // Pool 크기 제한 확인
                if (m_pool.size() < static_cast<size_t>(m_poolSize)) {
                    m_pool.push_back(std::move(actor));
                }
                // Pool이 가득 차면 그냥 삭제 (unique_ptr이 자동으로 해제)

                break;
            }
        }
    }

}