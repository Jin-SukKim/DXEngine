#include "pch.h"
#include "FireEffect.h"
namespace DE {
	FireEffect::FireEffect(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\TempEffect.json");
		m_particle->SetTarget(this);
	}

	FireEffect::~FireEffect()
	{
		// m_particle은 부모 ~EffectActor()에서 해제됨
	}
}