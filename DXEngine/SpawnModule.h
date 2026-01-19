#pragma once
#include "ParticleModule.h"
#include "ComputeShader.h"

namespace DE {

class SpawnModule : public ParticleModule
{
public:
	void Initialize(ParticleInitContext& ctx);
	void OnSpawn(SimulationContext& ctx) override;
	void PreUpdate(SimulationContext& ctx) override;
	ModulePriority GetPriority() override { return ModulePriority::Spawn; }
	void LoadFromJson(const json& data) override;
public:
	Vector3 localPos = Vector3(0.f);
	Vector3 spawnVolume = Vector3(0.05f, 0.15f, 0.05f);
	float spawnInnerRatio = 0.f;
	int spawnShape = 0;
	float spawnRate = 50.f;
	UINT particlesPerSpawn = 10;
	UINT maxParticles = 1024;
	Vector2 lifeRange = { 0.1f, 1.5f };
	float spawnAccumulator = 0.f;

private:
	ComputeShader m_spawnCS;
	UINT m_totalSpawnCount = 0;
};
}

