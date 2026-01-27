#include "pch.h"
#include "VisualEffectsActor.h"

namespace DE {
	VisualEffectsActor::VisualEffectsActor(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\VisualModule\\VisualColor.json");
		m_particle->SetTarget(this);
		
		m_rotation = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\VisualModule\\VisualRotation.json");
		m_rotation->SetTarget(this);
	}

	VisualEffectsActor::~VisualEffectsActor()
	{
		// m_particle은 부모 ~EffectActor()에서 해제됨
		ParticleManager::Get().DestroyInstance(m_rotation);
	}

	void VisualEffectsActor::Update(const float& deltaTime)
	{
		Super::Update(deltaTime);
	}
}