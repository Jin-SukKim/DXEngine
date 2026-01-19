#include "pch.h"
#include "SpawnModule.h"
#include "ParticleEmitter.h"

namespace DE {
	void SpawnModule::Initialize(ParticleInitContext& ctx)
	{
		m_spawnConsts.Initialize();

		ctx.consts.maxParticles = maxParticles;
		m_spawnCS.Initialize(ctx.device, L"SpawnCS.hlsl");
	}

	void SpawnModule::OnSpawn(SimulationContext& ctx)
	{
		ParticleModule::OnSpawn(ctx);
		SpawnConsts& consts = m_spawnConsts.GetCpu();
		consts.localPos = localPos;
		consts.spawnVolume = spawnVolume;
		consts.spawnInnerRatio = spawnInnerRatio;
		consts.spawnShape = spawnShape;
		consts.lifeRange = lifeRange;

		m_spawnConsts.Upload();
		ctx.context->CSSetConstantBuffers(5, 1, m_spawnConsts.GetAddressOf());

		ctx.constBuffer.GetCpu().maxParticles = maxParticles;
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

		ctx.constBuffer.GetCpu().spawnCount = m_totalSpawnCount;

		if (m_totalSpawnCount == 0)
			return;

		ctx.constBuffer.Upload();

		m_spawnCS.UpdateConsts(ctx.context, 4, 1, ctx.constBuffer.GetAddressOf());

		ID3D11UnorderedAccessView* uav = ctx.consumeBuffer.GetUAV();
		ctx.context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);

		// Spawn Compute Shader
		UINT groupCount = (m_totalSpawnCount + 255) / 256;
		m_spawnCS.Dispatch(ctx.context, groupCount, 1, 1);
	}

	void SpawnModule::LoadFromJson(const json& data)
	{
		if (data.contains("localPos")) localPos = JsonToVec3(data["localPos"]);
		if (data.contains("spawnVolume")) spawnVolume = JsonToVec3(data["spawnVolume"]);
		if (data.contains("spawnInnerRatio")) spawnInnerRatio = data["spawnInnerRatio"];
		if (data.contains("shape")) {
			std::string shape = data["shape"];
			if (shape == "Sphere") spawnShape = 1;
			else if (shape == "Box") spawnShape = 0;
		}
		if (data.contains("spawnRate")) spawnRate = data["spawnRate"];
		if (data.contains("particlesPerSpawn")) particlesPerSpawn = data["particlesPerSpawn"];
		if (data.contains("maxParticles")) maxParticles = data["maxParticles"];
		if (data.contains("lifeRange")) lifeRange = JsonToVec2(data["lifeRange"]);
	}
}