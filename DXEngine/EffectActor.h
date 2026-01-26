#pragma once
#include "Actor.h"
#include "ParticleSystem.h"

namespace DE {

    class EffectActor : public Actor
    {
        using Super = Actor;
    public:
        EffectActor(const std::wstring& name);
        virtual ~EffectActor() override;

        void Initialize() override;
        void Update(const float& deltaTime) override;
        void Render() override;

        // 파티클 시스템 설정 함수
        virtual void SetParticlePreset(const std::vector<std::wstring>& path);
        std::vector<ParticleSystem*>&  GetParticleSystem() { return m_particles; }
        void Stop();
        void Clear();
    private:
        std::vector<ParticleSystem*> m_particles;
    };

}