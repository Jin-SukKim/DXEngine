#include "pch.h"
#include "PortalEffect.h"

namespace DE {
	PortalEffect::PortalEffect(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\Combination\\Portal\\System_DimensionPortal.json");
		m_particle->SetTarget(this);
	}

	PortalEffect::~PortalEffect()
	{
		// m_particle은 부모 ~EffectActor()에서 해제됨
	}
}