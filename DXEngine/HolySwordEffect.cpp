#include "pch.h"
#include "HolySwordEffect.h"

namespace DE {
	HolySwordEffect::HolySwordEffect(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\Combination\\HolySword\\System_HolySword.json");
		m_particle->SetTarget(this);
	}

	HolySwordEffect::~HolySwordEffect()
	{
		// m_particle은 부모 ~EffectActor()에서 해제됨
	}
}