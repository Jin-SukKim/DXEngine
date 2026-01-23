#include "pch.h"
#include "ParticleSystemComponent.h"
#include "ParticleManager.h"
#include "MeshData.h"

namespace DE {
	ParticleSystemComponent::ParticleSystemComponent(const std::wstring& name) : Component(name, ComponentType::ParticleSystem), m_system(nullptr)
	{
	}

	ParticleSystemComponent::~ParticleSystemComponent()
	{
	}

	void ParticleSystemComponent::Initialize()
	{
		Component::Initialize();
		m_system->Initialize();
	}

	void ParticleSystemComponent::Update(const float& dt)
	{
		Component::Update(dt);
		m_system->Update(dt);
	}

	void ParticleSystemComponent::Render()
	{
		Component::Render();
		m_system->Render();
	}

	void ParticleSystemComponent::SetSystem(const std::wstring& path, const MeshData* meshData)
	{
		m_system = ParticleManager::Get().CreateSystem(path);
		m_system->SetTargetMesh(*meshData);
	}

	ParticleSystem* ParticleSystemComponent::GetSystem()
	{
		return m_system;
	}
}