#include "pch.h"
#include "PhoenixEffect.h"

namespace DE {
	PhoenixEffect::PhoenixEffect(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\Combination\\Phoenix\\System_PhoenixRebirth.json");
		m_particle->SetTarget(this);
	}

	PhoenixEffect::~PhoenixEffect()
	{
		// m_particle은 부모 ~EffectActor()에서 해제됨
	}
}