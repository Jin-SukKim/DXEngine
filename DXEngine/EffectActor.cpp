#include "pch.h"
#include "EffectActor.h"
#include "ParticleManager.h"
#include "TransformComponent.h"

namespace DE {

    EffectActor::EffectActor(const std::wstring& name) : Super(name)
    {
    }

    EffectActor::~EffectActor()
    {
        if (m_particle) {
            ParticleManager::Get().DestroyInstance(m_particle);
            m_particle = nullptr;
        }
    }

    void EffectActor::Initialize()
    {
        Super::Initialize();
        if (m_particle) m_particle->Initialize();
    }

    void EffectActor::Update(const float& deltaTime)
    {
        Super::Update(deltaTime);
        // ParticleManager가 Update를 담당하므로 여기서는 호출하지 않음
    }

    void EffectActor::Render()
    {
        Super::Render();
        // ParticleManager가 Render를 담당하므로 여기서는 호출하지 않음
    }

    void EffectActor::SetParticlePreset(const std::wstring& path)
    {
        // 기존 시스템 정리
        if (m_particle) {
            ParticleManager::Get().DestroyInstance(m_particle);
            m_particle = nullptr;
        }

        // 새 시스템 생성 (CreateSystem 내부에서 Initialize + OnSpawn 완료됨)
        m_particle = ParticleManager::Get().CreateSystem(path);

        if (m_particle) {
            m_particle->SetTarget(this);
            m_particle->Play();
        }
    }

    void EffectActor::Stop()
    {
        if (m_particle) {
            m_particle->Stop();
        }
    }

    bool EffectActor::IsPlaying() const
    {
        return m_particle && m_particle->IsPlaying();
    }

    bool EffectActor::IsFinished() const
    {
        if (!m_particle)
            return true;

        // Stopped 상태이면 완료
        if (m_particle->IsStopped())
            return true;

        // Looping이 아니고 모든 Emitter가 완료되면 완료
        if (!m_particle->IsLooping() && m_particle->IsAllEmittersCompleted())
            return true;

        return false;
    }
}