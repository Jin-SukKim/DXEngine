#include "pch.h"
#include "NightSkyEffect.h"

namespace DE {

	NightSkyEffect::NightSkyEffect(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\Combination\\NightSky\\System_CosmicNightSky.json");
		m_particle->SetTarget(this);
	}

	NightSkyEffect::~NightSkyEffect()
	{
		// m_particle은 부모 ~EffectActor()에서 해제됨
	}
}