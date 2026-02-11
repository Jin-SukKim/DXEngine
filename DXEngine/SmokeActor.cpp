#include "pch.h"
#include "SmokeActor.h"
namespace DE {
	SmokeActor::SmokeActor(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\SmokeEffect.json");
		m_particle->SetTarget(this);
	}

	SmokeActor::~SmokeActor()
	{
		// m_particle은 부모 ~EffectActor()에서 해제됨
	}
}