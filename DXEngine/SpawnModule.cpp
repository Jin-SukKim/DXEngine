#include "pch.h"
#include "SpawnModule.h"
#include "ParticleEmitter.h"

namespace DE {
	void SpawnModule::Initialize(ParticleInitContext& ctx)
	{
		ctx.frameConsts.maxParticles = m_maxParticles;
		
		SpawnConsts& consts = ctx.consts.spawn;
		consts.localPos = m_localPos;
		consts.spawnVolume = m_spawnVolume;
		consts.spawnInnerRatio = m_spawnInnerRatio;
		consts.spawnShape = m_spawnShape;
		consts.lifeRange = m_lifeRange;
		consts.simulationSpace = m_simulationSpace;

		if (m_spawnShape == 5) // Custom Mode
		{
			ctx.customPositions = m_customPositions;
			ctx.consts.spawn.bakedCount = (UINT)m_customPositions.size();
			ctx.usingCustomPositions = true;
			ctx.consts.spawn.spawnStartIndex = 0;
		}

		ctx.frameConsts.maxParticles = m_maxParticles;
	}

	void SpawnModule::OnPreUpdate(const SimulationContext& ctx)
	{
		ParticleModule::OnPreUpdate(ctx);
		if (m_burstCount > 0 && !m_burstFired)
		{
			m_spawnAccumulator += (float)m_burstCount;
			m_burstFired = true;
		}

		if (m_spawnRate > 0.0f)
		{
			m_spawnAccumulator += m_spawnRate * ctx.dt;
		}

		m_totalSpawnCount = 1;
		UINT spawnCycles = static_cast<int>(m_spawnAccumulator);
		m_totalSpawnCount = spawnCycles * m_particlesPerSpawn;

		if (spawnCycles > 0)
			m_spawnAccumulator -= static_cast<float>(spawnCycles);

		ctx.fsConsts->spawnCount = m_totalSpawnCount;
		ctx.fsConsts->maxParticles = m_maxParticles;
	}

	void SpawnModule::LateUpdate(SimulationContext& ctx)
	{
		ParticleModule::LateUpdate(ctx);
		
		if (m_totalSpawnCount == 0)
			return;

		// CSSetShader는 ParticleManager에서 루프 밖에서 한 번만 호출
		UINT groupCount = (m_totalSpawnCount + 1023) >> 10;
		ctx.context->Dispatch(groupCount, 1, 1);
	}

	void SpawnModule::LoadFromJson(const json& data)
	{
		if (data.contains("space")) m_simulationSpace = data["space"] == "World";
		if (data.contains("localPos")) m_localPos = JsonToVec3(data["localPos"]);
		if (data.contains("spawnVolume")) m_spawnVolume = JsonToVec3(data["spawnVolume"]);
		if (data.contains("spawnInnerRatio")) m_spawnInnerRatio = data["spawnInnerRatio"];
		if (data.contains("shape")) {
			std::string shape = data["shape"];
			if (shape == "Box") m_spawnShape = 0;
			else if (shape == "Sphere") m_spawnShape = 1;
			else if (shape == "Vertex") m_spawnShape = 2;
			else if (shape == "Surface") m_spawnShape = 3;
			else if (shape == "Texture") m_spawnShape = 4;
			else if (shape == "Custom") {
				m_spawnShape = 5;
				if (data.contains("positions") && data["positions"].is_array()) {
					std::vector<Vector3> positions;
					for (const auto& item : data["positions"]) {
						positions.push_back(JsonToVec3(item));
					}
					SetSpawnPosition(positions);
				}
			}
		}
		if (data.contains("spawnRate")) m_spawnRate = data["spawnRate"];
		if (data.contains("burst")) m_burstCount = data["burst"];
		if (data.contains("particlesPerSpawn")) m_particlesPerSpawn = data["particlesPerSpawn"];
		if (data.contains("maxParticles")) m_maxParticles = data["maxParticles"];
		if (data.contains("lifeRange")) m_lifeRange = JsonToVec2(data["lifeRange"]);
	}

	std::unique_ptr<ParticleModule> SpawnModule::Clone() const
	{
		auto cloned = std::make_unique<SpawnModule>();

		cloned->m_localPos = this->m_localPos;
		cloned->m_spawnVolume = this->m_spawnVolume;
		cloned->m_spawnInnerRatio = this->m_spawnInnerRatio;
		cloned->m_spawnShape = this->m_spawnShape;
		cloned->m_spawnRate = this->m_spawnRate;
		cloned->m_particlesPerSpawn = this->m_particlesPerSpawn;
		cloned->m_maxParticles = this->m_maxParticles;
		cloned->m_lifeRange = this->m_lifeRange;
		cloned->m_simulationSpace = this->m_simulationSpace;
		cloned->m_isEnabled = this->m_isEnabled;
		cloned->m_burstCount = this->m_burstCount;
		cloned->m_customPositions = this->m_customPositions;

		return cloned;
	}

	void SpawnModule::SetSpawnPosition(const std::vector<Vector3>& positions)
	{
		m_customPositions = positions;
		m_spawnShape = 5;
		m_nextSpawnIndex = 0;
	}

	void SpawnModule::OnSpawn(SimulationContext& ctx)
	{
		ParticleModule::OnSpawn(ctx);
		
		m_spawnAccumulator = 0.f;
		m_totalSpawnCount = 0;
		m_burstFired = false;
		m_nextSpawnIndex = 0;
	}
}