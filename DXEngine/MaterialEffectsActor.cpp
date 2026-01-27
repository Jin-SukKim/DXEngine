#include "pch.h"
#include "MaterialEffectsActor.h"
#include "ParticleManager.h"

namespace DE {
	MaterialEffectsActor::MaterialEffectsActor(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\MaterialModule\\MaterialPBR.json");
		m_particle->SetTarget(this);
	}

	MaterialEffectsActor::~MaterialEffectsActor()
	{
		// m_particle은 부모 ~EffectActor()에서 해제됨
	}

	void MaterialEffectsActor::Update(const float& deltaTime)
	{
		Super::Update(deltaTime);
	}
}