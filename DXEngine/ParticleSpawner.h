#pragma once
#include "Actor.h"
#include "EffectActor.h" // EffectActor�� NeedsExternalPreset() ����� ���� �ʿ�

namespace DE {
    class Scene;

    enum class SpawnMode {
        Continuous,
        Interval,
        OneShot,
        Burst
    };
    /**
     * ParticleSpawner - ObjectPool 패턴을 사용하는 파티클 이펙트 생성 관리자
     * 
     * [ObjectPool 구현]
     * - GPU Buffer 재사용을 위해 EffectActor를 미리 생성하고 풀에 보관
     * - 완료된 Effect를 삭제하지 않고 풀로 반환하여 재활용
     * - 기본 풀 크기: 20개 (SetPoolSize로 변경 가능)
     * 
     * [동작 방식]
     * 1. Initialize 시 m_poolSize만큼 EffectActor를 미리 생성
     * 2. Spawn 호출 시 풀에서 가져와서 재활용 (풀이 비면 동적 생성)
     * 3. Effect 완료 시 Scene에서 회수하여 풀로 반환
     * 
     * [장점]
     * - GPU Buffer 생성 비용 최소화 (최초 1회만 생성)
     * - 메모리 할당/해제 빈도 감소로 성능 향상
     * - 리소스 재사용으로 메모리 효율 증대
     */
    // Effect ������ scene���� ��û (������ ����)
    class ParticleSpawner : public Actor {
        using Super = Actor;
    public:
        ParticleSpawner(const std::wstring& name);
        virtual ~ParticleSpawner() override;

        void Initialize() override;
        void Update(const float& deltaTime) override;
        void Render() override;

        void SetScene(Scene* scene) { m_scene = scene; }

        // [Spawner ����]
        void SetParticlePreset(const std::wstring& presetPath);
        void SetSpawnMode(SpawnMode mode);
        void SetSpawnInterval(float interval);
        void SetMaxActiveParticles(int maxCount);
        void SetSpawnBox(Vector3 halfExtends);
        void SetAutoDestroy(bool enable);
        void SetLifetime(float lifetime);
        void SetPoolSize(int size);  // ObjectPool 크기 설정

        // [���� ����]
        void Spawn();                   // ���� �Լ� (������)
        void SpawnBurst(int count);
        void Stop();

        // ���丮 �Լ� Ÿ�� ����
        using ActorFactory = std::function<std::unique_ptr<EffectActor>(const std::wstring&)>;

        // [����] ������ Actor Ÿ���� �����ϴ� ���ø� �Լ�
        template <typename T>
        void SetActorType() {
            static_assert(std::is_base_of<EffectActor, T>::value, "T must derive from EffectActor");

            // ������ Ÿ�� T(��: Firework)�� �����Ͽ� ��ȯ�ϴ� ���� ���
            m_actorFactory = [](const std::wstring& name) -> std::unique_ptr<EffectActor> {
                return std::make_unique<T>(name);
            };
        }

    private:
        void UpdateSpawning(float dt);
        void CleanupDeadSystems();
        Vector3 GetRandomSpawnPosition();
        
        // ObjectPool 관련 메서드
        void InitializePool();
        EffectActor* AcquireFromPool();
        void ReleaseToPool(EffectActor* effect);

        // �⺻���� EffectActor ����
        ActorFactory m_actorFactory = [](const std::wstring& name) {
            return std::make_unique<EffectActor>(name);
        };

    private:
        Scene* m_scene = nullptr;
        std::wstring m_presetPath;
        SpawnMode m_spawnMode = SpawnMode::Continuous;

        float m_spawnInterval = 1.0f;
        float m_spawnAccumulator = 0.0f;
        size_t m_maxActiveParticles = 10;
        Vector3 m_spawnBoxExtents = Vector3(1.f);

        bool m_autoDestroy = false;
        float m_lifetime = -1.0f;
        float m_elapsedTime = 0.0f;
        bool m_stopped = false;

        // ObjectPool 관련 멤버 변수
        int m_poolSize = 20;                                    // 기본 Pool 크기
        std::vector<std::unique_ptr<EffectActor>> m_pool;       // Pool에 보관된 EffectActor들
        std::vector<EffectActor*> m_activeEffects;              // 현재 활성화된 Effect들

        // EffectActor 관리 (호환성 유지)
        std::vector<EffectActor*> m_spawnedEffects;
    };

}