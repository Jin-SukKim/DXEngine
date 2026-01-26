#pragma once
#include "Actor.h"
#include "EffectActor.h" // EffectActor의 NeedsExternalPreset() 사용을 위해 필요
#include <functional>
#include <memory> // std::unique_ptr

namespace DE {

    enum class SpawnMode {
        Continuous,
        Interval,
        OneShot,
        Burst
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
        void Spawn();                   // 생성 함수 (수정됨)
        void SpawnBurst(int count);
        void Stop();
        void Clear();

        // 팩토리 함수 타입 정의
        using ActorFactory = std::function<std::unique_ptr<EffectActor>(const std::wstring&)>;

        // [설정] 생성할 Actor 타입을 지정하는 템플릿 함수
        template <typename T>
        void SetActorType() {
            static_assert(std::is_base_of<EffectActor, T>::value, "T must derive from EffectActor");

            // 지정된 타입 T(예: Firework)를 생성하여 반환하는 람다 등록
            m_actorFactory = [](const std::wstring& name) -> std::unique_ptr<EffectActor> {
                return std::make_unique<T>(name);
            };
        }

    private:
        void UpdateSpawning(float dt);
        void CleanupDeadSystems();
        Vector3 GetRandomSpawnPosition();

        // 기본값은 EffectActor 생성
        ActorFactory m_actorFactory = [](const std::wstring& name) {
            return std::make_unique<EffectActor>(name);
        };

    private:
        std::wstring m_presetPath;
        SpawnMode m_spawnMode = SpawnMode::Continuous;

        float m_spawnInterval = 1.0f;
        float m_spawnAccumulator = 0.0f;
        int m_maxActiveParticles = 10;
        float m_spawnRadius = 0.0f;

        bool m_autoDestroy = false;
        float m_lifetime = -1.0f;
        float m_elapsedTime = 0.0f;

        // unique_ptr로 소유권 관리
        std::vector<std::unique_ptr<EffectActor>> m_spawnedActors;
    };

}