#include "pch.h"
#include "EmitterModule.h"
#include "ParticleEmitter.h"

namespace DE {
	void EmitterModule::OnSpawn(ID3D11DeviceContext* context)
	{
		ParticleConsts& consts = m_owner->GetConsts();
		consts.spawnVolume = spawnVolume;
		consts.lifeRange = lifeRange;
		consts.maxParticles = maxParticles;
	}

	void EmitterModule::PreUpdate(ID3D11DeviceContext* context, float dt)
	{
		spawnAccumulator += spawnRate * dt;

		UINT spawnCycles = static_cast<int>(spawnAccumulator);
		UINT totalSpawnCount = spawnCycles * particlesPerSpawn;

		if (spawnCycles > 0)
			spawnAccumulator -= static_cast<float>(spawnCycles);
		if (totalSpawnCount <= 0) {
			m_canSpawn = false;
			return;
		}

		m_owner->GetConsts().spawnCount = totalSpawnCount;
		m_canSpawn = true;
	}
}