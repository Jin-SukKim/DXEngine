#pragma once
#include "ParticleModule.h"

namespace DE {

class EmitterModule : public ParticleModule
{
public:
	void OnSpawn(ID3D11DeviceContext* context) override;
	void PreUpdate(ID3D11DeviceContext* context, float dt) override;
	bool CanSpawn() { return m_canSpawn; }
	ModulePriority GetPriority() override { return ModulePriority::Spawn; }

public:
	Vector3 spawnVolume = Vector3(0.05f, 0.15f, 0.05f);
	float spawnRate = 50.f;
	UINT particlesPerSpawn = 10;
	UINT maxParticles = 1024;
	Vector2 lifeRange = { 0.1f, 1.5f };
	float spawnAccumulator = 0.f;
private:
	bool m_canSpawn = false;
};
}

