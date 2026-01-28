#include "pch.h"
#include "Firework.h"
#include "ParticleSystem.h"
#include "TransformComponent.h"
#include "ParticleManager.h"
namespace DE {
	Firework::Firework(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\Firework.json");
		if (m_particle)
			m_particle->SetTarget(this);
	}

	Firework::~Firework()
	{
		// m_particle은 부모 ~EffectActor()에서 해제됨
	}

	void Firework::Initialize()
	{
		Super::Initialize();

		TransformComponent* tr = this->GetComponent<TransformComponent>();
		if (tr) {
			tr->SetScale(Vector3(3.f));
		}
	}

	void Firework::Update(const float& deltaTime)
	{
		Super::Update(deltaTime);

		if (!m_particle || m_particle->IsStopped()) 
			return; 
		
		TransformComponent* tr = this->GetComponent<TransformComponent>();
		if (tr) {
			Vector3 pos = tr->GetPos();
			pos.y += 5.0f * deltaTime;
			tr->SetPos(pos);
		}
	}
}