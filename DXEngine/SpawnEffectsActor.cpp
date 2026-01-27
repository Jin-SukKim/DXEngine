#include "pch.h"
#include "SpawnEffectsActor.h"
#include "ParticleManager.h"

namespace DE {
	SpawnEffectsActor::SpawnEffectsActor(const std::wstring& name) : Super(name)
	{
		constexpr float spacing = 3.0f; // 각 시스템 간 간격

		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\SpawnModule\\SpawnBox.json");
		m_particle->SetTarget(this);
		m_particle->SetSpawnOffset(Vector3(-spacing * 2.5f, 0.f, 0.f));

		m_burst = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\SpawnModule\\SpawnBurst.json");
		m_burst->SetTarget(this);
		m_burst->SetSpawnOffset(Vector3(-spacing * 1.5f, 0.f, 0.f));

		m_custom = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\SpawnModule\\SpawnCustom.json");
		m_custom->SetTarget(this);
		m_custom->SetSpawnOffset(Vector3(-spacing * 0.5f, 0.f, 0.f));

		m_hollow = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\SpawnModule\\SpawnHollowSphere.json");
		m_hollow->SetTarget(this);
		m_hollow->SetSpawnOffset(Vector3(spacing * 0.5f, 0.f, 0.f));

		m_sphere = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\SpawnModule\\SpawnSphere.json");
		m_sphere->SetTarget(this);
		m_sphere->SetSpawnOffset(Vector3(spacing * 1.5f, 0.f, 0.f));

		m_texture = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\SpawnModule\\SpawnTexture.json");
		m_texture->SetTarget(this);
		m_texture->SetSpawnOffset(Vector3(spacing * 2.5f, 0.f, 0.f));
	}

	SpawnEffectsActor::~SpawnEffectsActor()
	{
		ParticleManager::Get().DestroyInstance(m_particle);
		ParticleManager::Get().DestroyInstance(m_burst);
		ParticleManager::Get().DestroyInstance(m_custom);
		ParticleManager::Get().DestroyInstance(m_hollow);
		ParticleManager::Get().DestroyInstance(m_sphere);
		ParticleManager::Get().DestroyInstance(m_texture);
	}

	void SpawnEffectsActor::Update(const float& deltaTime)
	{
		Super::Update(deltaTime);

		if (m_particle->IsStopped())
			return;
	}
}