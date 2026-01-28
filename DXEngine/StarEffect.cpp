#include "pch.h"
#include "StarEffect.h"

namespace DE {
	StarEffect::StarEffect(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\Combination\\Star\\System_StarBirth.json");
		m_particle->SetTarget(this);
	}

	StarEffect::~StarEffect()
	{
		// m_particle은 부모 ~EffectActor()에서 해제됨
	}
}