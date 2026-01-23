#pragma once
#include "ParticleSystem.h"

namespace DE {

class ParticleManager
{
public:
	static ParticleManager& Get() {
		static ParticleManager instance;
		return instance;
	}

	void Update(const float& dt);
	void Render();

	ParticleSystem* CreateSystem(const std::wstring& path);

private:
	std::unordered_map<std::wstring, std::unique_ptr<ParticleSystem>> m_particles;
};

}

