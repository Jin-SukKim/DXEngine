#pragma once
#include "ParticleModule.h"
#include "ComputeShader.h"

namespace DE {

class SpawnModule : public ParticleModule
{
public:
	void Initialize(ID3D11Device* device, ParticleEmitter* owner);
	void OnSpawn(ID3D11DeviceContext* context) override;
	void PreUpdate(ID3D11DeviceContext* context, float dt) override;
	void OnUpdate(ID3D11DeviceContext* context, float dt) override;
	ModulePriority GetPriority() override { return ModulePriority::Spawn; }
	void LoadFromJson(const json& data) override;
public:
	Vector3 spawnVolume = Vector3(0.05f, 0.15f, 0.05f);
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

