#pragma once
#include "Actor.h"
#include "EffectActor.h"
#include <functional>

namespace DE {

    class Scene;

    enum class SpawnMode {
        Continuous,
        Interval,
        OneShot,
        Burst
    };

    enum class SpawnShape {
        Circle,  // XZ 평면 원
        Sphere,  // 3D 구체
        Box      // 3D 박스
    };

    // ParticleSpawner: Effect 생성 요청만 담당 (소유권 없음)
    class ParticleSpawner : public Actor {
        using Super = Actor;
    public:
        ParticleSpawner(const std::wstring& name);
        virtual ~ParticleSpawner() override = default;

        void Initialize() override;
        void Update(const float& deltaTime) override;
        void Render() override;

        // Scene 설정 (필수)
        void SetOwnerScene(Scene* scene) { m_ownerScene = scene; }

        // [Spawner 설정]
        void SetParticlePreset(const std::wstring& presetPath);
        void SetSpawnMode(SpawnMode mode);
        void SetSpawnInterval(float interval);
        void SetMaxActiveParticles(int maxCount);
        void SetSpawnRadius(float radius);
        void SetSpawnShape(SpawnShape shape) { m_spawnShape = shape; }
        void SetSpawnBox(const Vector3& halfExtents) { m_spawnBoxExtents = halfExtents; }
        void SetAutoDestroy(bool enable);
        void SetLifetime(float lifetime);

        // [생성 제어]
        void Spawn();
        void SpawnBurst(int count);
        void Stop();

        // 팩토리 함수 타입 정의
        using ActorFactory = std::function<std::unique_ptr<EffectActor>(const std::wstring&)>;

        // [고급] 커스텀 Actor 타입을 지정하는 템플릿 함수
        template <typename T>
        void SetActorType() {
            static_assert(std::is_base_of_v<EffectActor, T>, "T must derive from EffectActor");
            m_actorFactory = [](const std::wstring& name) -> std::unique_ptr<EffectActor> {
                return std::make_unique<T>(name);
            };
        }

    private:
        void UpdateSpawning(float dt);
        void CleanupExpiredEffects();
        Vector3 GetRandomSpawnPosition();

        // 기본적으로 EffectActor 생성
        ActorFactory m_actorFactory = [](const std::wstring& name) {
            return std::make_unique<EffectActor>(name);
        };

    private:
        Scene* m_ownerScene = nullptr;
        std::wstring m_presetPath;
        SpawnMode m_spawnMode = SpawnMode::Continuous;
        SpawnShape m_spawnShape = SpawnShape::Circle;

        float m_spawnInterval = 1.0f;
        float m_spawnAccumulator = 0.0f;
        int m_maxActiveParticles = 10;
        float m_spawnRadius = 0.0f;
        Vector3 m_spawnBoxExtents = Vector3(1.0f);  // half extents

        bool m_autoDestroy = false;
        float m_lifetime = -1.0f;
        float m_elapsedTime = 0.0f;
        bool m_stopped = false;

        // 스폰된 Effect 추적 (raw pointer - 소유권 없음)
        std::vector<EffectActor*> m_spawnedEffects;
    };

}