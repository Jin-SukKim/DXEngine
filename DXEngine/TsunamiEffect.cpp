#include "pch.h"
#include "TsunamiEffect.h"
namespace DE {
	TsunamiEffect::TsunamiEffect(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\Combination\\Tsunami\\System_WaterDragon.json");
		m_particle->SetTarget(this);
	}

	TsunamiEffect::~TsunamiEffect()
	{
		// m_particle은 부모 ~EffectActor()에서 해제됨
	}
}