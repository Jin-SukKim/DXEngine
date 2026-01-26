#include "pch.h"
#include "Firework.h"
#include "ParticleSystem.h"
#include "TransformComponent.h"
#include "ParticleManager.h"
namespace DE {
	Firework::Firework(const std::wstring& name) : Super(name)
	{
		m_up = ParticleManager::Get().CreateSystem(L"..\\Assets\\Particles\\UpFIrework.json");
		m_up->SetTarget(this);

		m_firework = ParticleManager::Get().CreateSystem(L"..\\Assets\\Particles\\Firework.json");
		m_firework->SetTarget(this);
		m_firework->Stop();
	}

	void Firework::Initialize()
	{
		Super::Initialize();

	}

	void Firework::Update(const float& deltaTime)
	{
		Super::Update(deltaTime);
		if (m_up->IsStopped() && count)
			return;

		if (m_up->IsStopped()) {
			m_firework->Play();
			++count;
			return; 
		}

		TransformComponent* tr = this->GetComponent < TransformComponent>();
		if (tr) {
			Vector3 pos = tr->GetPos();
			pos.y += 1.f * deltaTime;
			tr->SetPos(pos);
		}

	}
}