#include "pch.h"
#include "EffectActor.h"
#include "ParticleSystem.h"
#include "ParticleManager.h"

namespace DE {
    EffectActor::EffectActor(const std::wstring& name) : Super(name)
    {
        m_particles = ParticleManager::Get().CreateSystem(L"Particles\\TempEffect.json");
        m_particles->SetTarget(this);
    }

    void EffectActor::Initialize()
    {
        Super::Initialize();
    }
	void EffectActor::Update(const float& deltaTime)
	{
		Super::Update(deltaTime);
	}
}