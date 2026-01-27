#include "pch.h"
#include "ForceEffectsActor.h"
#include "ParticleManager.h"

namespace DE {
	ForceEffectsActor::ForceEffectsActor(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\ForceModule\\ForceDrag.json");
		m_particle->SetTarget(this);

		m_fountain = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\ForceModule\\ForceFountain.json");
		m_fountain->SetTarget(this);

		m_gravity = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\ForceModule\\ForceGravity.json");
		m_gravity->SetTarget(this);
	}

	ForceEffectsActor::~ForceEffectsActor()
	{
		ParticleManager::Get().DestroyInstance(m_particle);
		ParticleManager::Get().DestroyInstance(m_fountain);
		ParticleManager::Get().DestroyInstance(m_gravity);
	}

	void ForceEffectsActor::Update(const float& deltaTime)
	{
		Super::Update(deltaTime);

	}
}