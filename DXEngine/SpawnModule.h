#pragma once
#include "ParticleModule.h"
#include "ComputeShader.h"
#include "MeshData.h"
namespace DE {

	struct Mesh;

class SpawnModule : public ParticleModule
{
public:
	void Initialize(ParticleInitContext& ctx);
	void OnSpawn(SimulationContext& ctx) override; 
	void OnUpdateCPU(SimulationContext& context) override;
	void PreUpdate(SimulationContext& ctx) override;
	ModulePriority GetPriority() override { return ModulePriority::Spawn; }
	void LoadFromJson(const json& data) override;

	void SetTarget(const MeshData& meshes);
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
	UINT m_vertexCount = 0;
	UINT m_indexCount = 0;
	UINT m_simulationSpace = 0; // 0 : local, 1 : world

	ComputeShader m_spawnCS;
	UINT m_totalSpawnCount = 0;
	StructuredBuffer<Vertex> m_meshVertex;
	StructuredBuffer<uint32_t> m_meshIndices;
	StructuredBuffer<Vector3> m_spawnPos;
	UINT m_bakedCount = 0;
	
	// For deferred buffer initialization
	std::string m_bakedPath;
	bool m_needsSpawnPosInit = false;
};
}

