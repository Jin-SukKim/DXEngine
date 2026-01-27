#include "pch.h"
#include "TestActor.h"
#include "ParticleSystem.h"
#include "TransformComponent.h"
#include "ParticleManager.h"
namespace DE {
	TestActor::TestActor(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\SpawnModule\\SpawnEffect.json");
		m_particle->SetTarget(this);
	}

	TestActor::~TestActor()
	{
		ParticleManager::Get().DestroyInstance(m_particle);
	}
}