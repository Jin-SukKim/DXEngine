#pragma once
#include "ParticleModule.h"

namespace DE {

class SpawnModule : public ParticleModule
{
public:
	void Initialize(ParticleInitContext& ctx);
	void OnSpawn(SimulationContext& ctx) override; 
	void OnUpdateCPU(SimulationContext& context) override;
	void PreUpdate(SimulationContext& ctx) override;
	ModulePriority GetPriority() override { return ModulePriority::Spawn; }
	void LoadFromJson(const json& data) override;

	std::unique_ptr<ParticleModule> Clone() const override;
	void SetSpawnPosition(const std::vector<Vector3>& positions);
private:
	Vector3 m_localPos = Vector3(0.f);
	Vector3 m_spawnVolume = Vector3(0.05f, 0.15f, 0.05f);
	float m_spawnInnerRatio = 0.f;
	int m_spawnShape = 0;
	float m_spawnRate = 50.f;
	UINT m_particlesPerSpawn = 10;
	UINT m_maxParticles = 1024;
	Vector2 m_lifeRange = { 0.1f, 1.5f };
	float m_spawnAccumulator = 0.f;
	UINT m_simulationSpace = 0; // 0 : local, 1 : world
	UINT m_totalSpawnCount = 0;
	UINT m_burstCount = 0;      // 한 번에 터트릴 개수
	bool m_burstFired = false; // 이미 터졌는지 체크
	UINT m_nextSpawnIndex = 0;
	std::vector<Vector3> m_customPositions;

	std::string m_bakedPath;
	bool m_needsSpawnPosInit = false;
};
}

