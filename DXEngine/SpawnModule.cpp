#include "pch.h"
#include "SpawnModule.h"
#include "ParticleEmitter.h"

namespace DE {
	void SpawnModule::Initialize(ParticleInitContext& ctx)
	{
		ParticleModule::Initialize(ctx);
		ctx.consts.maxParticles = maxParticles;
		m_spawnCS.Initialize(ctx.device, L"SpawnCS.hlsl");
	}

	void SpawnModule::OnSpawn(SimulationContext& ctx)
	{
		ParticleModule::OnSpawn(ctx);
		ctx.consts.spawnVolume = spawnVolume;
		ctx.consts.lifeRange = lifeRange;
		ctx.consts.maxParticles = maxParticles;
	}

	void SpawnModule::PreUpdate(SimulationContext& ctx)
	{
		ParticleModule::PreUpdate(ctx);
		spawnAccumulator += spawnRate * ctx.dt;

		UINT spawnCycles = static_cast<int>(spawnAccumulator);
		m_totalSpawnCount = spawnCycles * particlesPerSpawn;

		if (spawnCycles > 0)
			spawnAccumulator -= static_cast<float>(spawnCycles);
		if (m_totalSpawnCount < 0)
			m_totalSpawnCount = 0;

		ctx.consts.spawnCount = m_totalSpawnCount;
	}
	
	void SpawnModule::OnUpdate(const SimulationContext& ctx)
	{
		ParticleModule::OnUpdate(ctx);
		if (m_totalSpawnCount == 0)
			return;

		m_spawnCS.UpdateConsts(ctx.context, 0, 1, ctx.constBuffer.GetAddressOf());

		ID3D11UnorderedAccessView* uav = ctx.consumeBuffer.GetUAV();
		ctx.context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);

		// Spawn Compute Shader
		UINT groupCount = (m_totalSpawnCount + 255) / 256;
		m_spawnCS.Dispatch(ctx.context, groupCount, 1, 1);
	}

	void SpawnModule::LoadFromJson(const json& data)
	{
		if (data.contains("spawnVolume")) spawnVolume = JsonToVec3(data["spawnVolume"]);
		if (data.contains("spawnRate")) spawnRate = data["spawnRate"];
		if (data.contains("particlesPerSpawn")) particlesPerSpawn = data["particlesPerSpawn"];
		if (data.contains("maxParticles")) maxParticles = data["maxParticles"];
		if (data.contains("lifeRange")) lifeRange = JsonToVec2(data["lifeRange"]);
	}
}