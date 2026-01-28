#include "pch.h"
#include "BreathEffect.h"


namespace DE {
	BreathEffect::BreathEffect(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\Combination\\DragonBreath\\System_DragonBreath.json");
		m_particle->SetTarget(this);
	}

	BreathEffect::~BreathEffect()
	{
		// m_particle은 부모 ~EffectActor()에서 해제됨
	}
}