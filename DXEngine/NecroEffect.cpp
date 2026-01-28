#include "pch.h"
#include "NecroEffect.h"
namespace DE {

	NecroEffect::NecroEffect(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\Combination\\Necro\\System_NecroSummon.json");
		m_particle->SetTarget(this);
	}

	NecroEffect::~NecroEffect()
	{
		// m_particle은 부모 ~EffectActor()에서 해제됨
	}
}