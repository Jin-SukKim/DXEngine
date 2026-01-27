#include "pch.h"
#include "BillboardEffectsActor.h"
#include "ParticleManager.h"

namespace DE {
	BillboardEffectsActor::BillboardEffectsActor(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\SpawnModule\\SpawnBox.json");
		m_particle->SetTarget(this);

		m_burst = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\SpawnModule\\SpawnBurst.json");
		m_burst->SetTarget(this);

		m_custom = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\SpawnModule\\SpawnCustom.json");
		m_custom->SetTarget(this);

		m_hollow = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\SpawnModule\\SpawnHollowSphere.json");
		m_hollow->SetTarget(this);

		m_sphere = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\SpawnModule\\SpawnSphere.json");
		m_sphere->SetTarget(this);

		m_texture = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\SpawnModule\\SpawnTexture.json");
		m_texture->SetTarget(this);
	}

	BillboardEffectsActor::~BillboardEffectsActor()
	{
		ParticleManager::Get().DestroyInstance(m_particle);
	}

	void BillboardEffectsActor::Update(const float& deltaTime)
	{
		Super::Update(deltaTime);

		if (m_particle->IsStopped())
			return;
	}
}