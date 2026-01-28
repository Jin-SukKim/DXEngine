#include "pch.h"
#include "OrbitEffectsActor.h"
namespace DE {
	OrbitEffectsActor::OrbitEffectsActor(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\OrbitModule\\OrbitBasic.json");
		m_particle->SetTarget(this);
		m_fastTrail = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\OrbitModule\\OrbitFastTrail.json");
		m_fastTrail->SetTarget(this);
		m_spiral = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\OrbitModule\\OrbitSpiral.json");
		m_spiral->SetTarget(this);
		m_tilted = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\OrbitModule\\OrbitTilted.json");
		m_tilted->SetTarget(this);
		m_orbitX = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\OrbitModule\\OrbitX.json");
		m_orbitX->SetTarget(this);
	}

	OrbitEffectsActor::~OrbitEffectsActor()
	{
		// m_particle은 부모 ~EffectActor()에서 해제됨
		ParticleManager::Get().DestroyInstance(m_fastTrail);
		ParticleManager::Get().DestroyInstance(m_spiral);
		ParticleManager::Get().DestroyInstance(m_tilted);
		ParticleManager::Get().DestroyInstance(m_orbitX);
	}

	void OrbitEffectsActor::Update(const float& deltaTime)
	{
		Super::Update(deltaTime);
	}
}