#include "pch.h"
#include "Firework.h"
#include "ParticleSystem.h"
#include "TransformComponent.h"
#include "ParticleManager.h"
namespace DE {
	Firework::Firework(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\Firework.json");
		m_particle->SetTarget(this);
	}

	Firework::~Firework()
	{
		ParticleManager::Get().DestroyInstance(m_particle);
	}

	void Firework::Initialize()
	{
		Super::Initialize();
	}

	void Firework::Update(const float& deltaTime)
	{
		Super::Update(deltaTime);

		if (m_particle->IsStopped()) 
			return; 
		
		TransformComponent* tr = this->GetComponent<TransformComponent>();
		if (tr) {
			Vector3 pos = tr->GetPos();
			pos.y += 1.0f * deltaTime;
			tr->SetPos(pos);
		}

	}
}