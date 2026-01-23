#include "pch.h"
#include "ParticleManager.h"
#include "ParticleLoader.h"

namespace DE {
	void ParticleManager::Update(const float& dt)
	{
		for (const auto& [path, system] : m_particles) {
			system->Update(dt);
		}
	}
	void ParticleManager::Render()
	{
		for (const auto& [path, system] : m_particles) {
			system->Render();
		}
	}

	ParticleSystem* ParticleManager::CreateSystem(const std::wstring& path)
	{
		auto it = m_particles.find(path);
		if (it != m_particles.end())
			return it->second.get();

		auto newSystem = ParticleLoader::Load<ParticleSystem>(path);
		if (newSystem) {
			//newSystem->Initialize();
			m_particles[path] = std::move(newSystem);
			return m_particles[path].get();
		}
		
		return nullptr;
	}
}