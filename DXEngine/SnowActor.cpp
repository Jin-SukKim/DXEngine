#include "pch.h"
#include "SnowActor.h"
#include "ParticleManager.h"
#include "TransformComponent.h"

namespace DE {
	SnowActor::SnowActor(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\SpawnModule\\SpawnWorldSpace.json");
		m_particle->SetTarget(this);
	}

	SnowActor::~SnowActor()
	{
		ParticleManager::Get().DestroyInstance(m_particle);
	}

	void SnowActor::Update(const float& deltaTime)
	{
		Super::Update(deltaTime);

		if (m_particle->IsStopped())
			return;

		auto* tr = this->GetComponent<TransformComponent>();
		if (tr) {
			Vector3 pos = tr->GetPos();
			pos.x += 1.f * deltaTime;
			tr->SetPos(pos);
		}
	}
}