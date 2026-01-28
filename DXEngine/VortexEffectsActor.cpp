#include "pch.h"
#include "VortexEffectsActor.h"

namespace DE {
	VortexEffectsActor::VortexEffectsActor(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\VortexModule\\VortexGalaxy.json");
		if (m_particle)
			m_particle->SetTarget(this);

		m_tornado = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\VortexModule\\VortexTornado.json");
		if (m_tornado)
			m_tornado->SetTarget(this);
	}

	VortexEffectsActor::~VortexEffectsActor()
	{
		if (m_tornado) {
			ParticleManager::Get().DestroyInstance(m_tornado);
			m_tornado = nullptr;
		}
	}

	void VortexEffectsActor::Update(const float& deltaTime)
	{
		Super::Update(deltaTime);
	}
}