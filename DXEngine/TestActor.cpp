#include "pch.h"
#include "TestActor.h"
#include "ParticleSystem.h"
#include "TransformComponent.h"
#include "ParticleManager.h"
namespace DE {
	TestActor::TestActor(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\SpawnModule\\SpawnEffect.json");
		if (m_particle)
			m_particle->SetTarget(this);
	}

	TestActor::~TestActor()
	{
		// m_particle은 부모 ~EffectActor()에서 해제됨
	}
}