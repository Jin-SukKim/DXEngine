#pragma once
#include "Actor.h"
#include "ParticleSystem.h"
#include "ParticleManager.h"

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
        void Play();
        void Stop();
        bool IsPlaying() const;

        // 수명 관리용 가상 함수
        bool IsFinished() const;
        virtual bool NeedsExternalPreset() const { return true; }

        void SetPosOffset(Vector3 offset);
        bool FailedToCreate() { return m_particle == nullptr; }
    protected:
        ParticleSystem* m_particle = nullptr;
    };
}