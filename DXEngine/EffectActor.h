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

        virtual void SetParticlePreset(const std::wstring& path);

        ParticleSystem* GetParticleSystem() const { return m_particle; }

        // 헬퍼 함수
        void Stop();
        bool IsPlaying() const;

        // 수명 관리용 가상 함수
        virtual bool IsFinished() const;
        virtual bool NeedsExternalPreset() const { return true; }

    protected:
        ParticleSystem* m_particle = nullptr;
    };
}