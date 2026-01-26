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
        // 소멸 시 매니저에게 반납 요청
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
        // Actor의 Transform을 파티클에 동기화할 필요가 있다면 여기서 수행
        // (보통 ParticleSystem::Update 내부나 SetTarget에서 처리됨)

        if (m_particle) {
            m_particle->Update(deltaTime);
        }
    }

    void EffectActor::Render()
    {
        Super::Render();
        if (m_particle) {
            m_particle->Render();
        }
    }

    void EffectActor::SetParticlePreset(const std::wstring& path)
    {
        // 기존 시스템 정리
        if (m_particle) {
            ParticleManager::Get().DestroyInstance(m_particle);
            m_particle = nullptr;
        }

        // 새 시스템 생성
        m_particle = ParticleManager::Get().CreateSystem(path);

        if (m_particle) {
            m_particle->SetTarget(this);
            m_particle->Initialize();
            m_particle->OnSpawn();
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
        // 기본 로직: 파티클이 없거나 멈췄으면 끝난 것
        if (m_particle) {
            return m_particle->IsStopped();
        }
        return true;
    }
}