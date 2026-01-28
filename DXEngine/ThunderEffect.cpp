#include "pch.h"
#include "ThunderEffect.h"

namespace DE {
	ThunderEffect::ThunderEffect(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\Combination\\Thunder\\System_ThunderStrike.json");
		m_particle->SetTarget(this);
	}

	ThunderEffect::~ThunderEffect()
	{
		// m_particle은 부모 ~EffectActor()에서 해제됨
	}
}