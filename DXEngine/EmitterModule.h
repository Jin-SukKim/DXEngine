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
	Vector3 spawnVolume = Vector3(0.25f, 0.35f, 0.25f);
	float spawnRate = 50.f;
	UINT particlesPerSpawn = 10;
	UINT maxParticles = 1024;
	Vector2 lifeRange = { 0.3f, 1.f };
	float spawnAccumulator = 0.f;
private:
	bool m_canSpawn = false;
};
}

