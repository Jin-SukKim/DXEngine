#include "pch.h"
#include "MagicEffect.h"


namespace DE {
	MagicEffect::MagicEffect(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\Magic\\System_MagicCast.json");
		m_particle->SetTarget(this);
	}

	MagicEffect::~MagicEffect()
	{
		// m_particle은 부모 ~EffectActor()에서 해제됨
	}
}