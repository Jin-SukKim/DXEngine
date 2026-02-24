#include "pch.h"
#include "ExplosionEffect.h"

namespace DE {
	ExplosionEffect::ExplosionEffect(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\Explosion\\Explosion.json");
		m_particle->SetTarget(this);
	}

	ExplosionEffect::~ExplosionEffect()
	{
		// m_particle은 부모 ~EffectActor()에서 해제됨
	}
}