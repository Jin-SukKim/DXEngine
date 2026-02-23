#include "pch.h"
#include "SmokeEffect.h"

namespace DE {
	SmokeEffect::SmokeEffect(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\SmokeEffect.json");
		m_particle->SetTarget(this);
	}

	SmokeEffect::~SmokeEffect()
	{
		// m_particle은 부모 ~EffectActor()에서 해제됨
	}
}