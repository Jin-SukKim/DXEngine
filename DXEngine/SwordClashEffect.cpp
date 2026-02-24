#include "pch.h"
#include "SwordClashEffect.h"

namespace DE {
	SwordClashEffect::SwordClashEffect(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\Combination\\SwordClash\\System_SwordClash.json");
		m_particle->SetTarget(this);
	}

	SwordClashEffect::~SwordClashEffect()
	{
		// m_particle은 부모 ~EffectActor()에서 해제됨
	}
}