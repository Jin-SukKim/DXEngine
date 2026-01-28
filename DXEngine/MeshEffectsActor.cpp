#include "pch.h"
#include "MeshEffectsActor.h"

namespace DE {
	MeshEffectsActor::MeshEffectsActor(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\MeshRenderModule\\BasicBox.json");
		m_particle->SetTarget(this);
		m_sphere = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\MeshRenderModule\\BasicSphere.json");
		m_sphere->SetTarget(this);
		m_model = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\MeshRenderModule\\CustomModel.json");
		m_model->SetTarget(this);
	}

	MeshEffectsActor::~MeshEffectsActor()
	{
		// m_particle은 부모 ~EffectActor()에서 해제됨
		ParticleManager::Get().DestroyInstance(m_sphere);
		ParticleManager::Get().DestroyInstance(m_model);
	}
}