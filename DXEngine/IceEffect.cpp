#include "pch.h"
#include "IceEffect.h"

namespace DE {
	IceEffect::IceEffect(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\Combination\\Ice\\System_IceExplosion.json");
		m_particle->SetTarget(this);
	}

	IceEffect::~IceEffect()
	{
		// m_particle은 부모 ~EffectActor()에서 해제됨
	}
}