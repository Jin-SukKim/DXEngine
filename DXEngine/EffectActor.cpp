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
        // Actor가 파괴될 때 파티클 시스템도 정리 요청
        Clear();
    }

    void EffectActor::Initialize()
    {
        Super::Initialize();
    }

    void EffectActor::Update(const float& deltaTime)
    {
        Super::Update(deltaTime);
    }

    void EffectActor::Render()
    {
        Super::Render();
    }

    void EffectActor::SetParticlePreset(const std::vector<std::wstring>& path)
    {
        // 기존 시스템이 있다면 제거
        if (!m_particles.empty()) {
            Clear();
        }

        for (int i = 0; i < path.size(); ++i) {
            // 새 시스템 생성
            ParticleSystem* particle = ParticleManager::Get().CreateSystem(path[i]);

            // 이 Actor를 타겟으로 설정 (Transform 동기화)
            if (particle) {
                particle->SetTarget(this);
                particle->Initialize();
                particle->OnSpawn();
                particle->Play();
            }
            m_particles.push_back(particle);
        }
    }
    void EffectActor::Stop()
    {
        for (auto& particle : m_particles)
            particle->Stop();
    }
    void EffectActor::Clear()
    {
        if (!m_particles.empty()) {
            for (auto& particle : m_particles)
                ParticleManager::Get().DestroyInstance(particle);
            m_particles.clear();
        }
    }
}