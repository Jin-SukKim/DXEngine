#include "pch.h"
#include "SubEmitterEffectsActor.h"

namespace DE {
	SubEmitterEffectsActor::SubEmitterEffectsActor(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\SubEmitter\\SystemExplosion.json");
		m_particle->SetTarget(this);
	}

	SubEmitterEffectsActor::~SubEmitterEffectsActor()
	{
		// m_particle은 부모 ~EffectActor()에서 해제됨
	}

	void SubEmitterEffectsActor::Update(const float& deltaTime)
	{
		Super::Update(deltaTime);
	}
}