#include "pch.h"
#include "SpawnModule.h"
#include "ParticleEmitter.h"

namespace DE {
	void SpawnModule::Initialize(ParticleInitContext& ctx)
	{
		ctx.frameConsts.maxParticles = m_maxParticles;
		// ComputeShader는 ComputeCommon에서 공유
	}

	void SpawnModule::OnSpawn(SimulationContext& ctx)
	{
		ParticleModule::OnSpawn(ctx);
		SpawnConsts& consts = ctx.constBuffer.GetCpu().spawn;
		consts.localPos = m_localPos;
		consts.spawnVolume = m_spawnVolume;
		consts.spawnInnerRatio = m_spawnInnerRatio;
		consts.spawnShape = m_spawnShape;
		consts.lifeRange = m_lifeRange;
		consts.vertexCount = ctx.vertexCount;
		consts.indexCount = ctx.indexCount;
		consts.bakedCount = ctx.bakedCount;
		consts.simulationSpace = m_simulationSpace;

		ctx.frameConstBuffer.GetCpu().maxParticles = m_maxParticles;
	}

	void SpawnModule::OnUpdateCPU(SimulationContext& ctx)
	{
		ParticleModule::OnUpdateCPU(ctx);
		m_spawnAccumulator += m_spawnRate * ctx.dt;

		UINT spawnCycles = static_cast<int>(m_spawnAccumulator);
		m_totalSpawnCount = spawnCycles * m_particlesPerSpawn;

		if (spawnCycles > 0)
			m_spawnAccumulator -= static_cast<float>(spawnCycles);
		if (m_totalSpawnCount < 0)
			m_totalSpawnCount = 0;

		ctx.frameConstBuffer.GetCpu().spawnCount = m_totalSpawnCount;
	}

	void SpawnModule::PreUpdate(SimulationContext& ctx)
	{
		ParticleModule::PreUpdate(ctx);
		
		if (m_totalSpawnCount == 0)
			return;

		ID3D11UnorderedAccessView* uav = ctx.consumeBuffer.GetUAV();
		ctx.context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);

		if (m_spawnShape == 2 || m_spawnShape == 3) {
			// Vertex/Surface Spawn: ParticleEmitter의 메쉬 데이터 사용
			if (ctx.meshVertex && ctx.meshIndices) {
				ID3D11ShaderResourceView* srvs[] = {
					ctx.meshVertex->GetSRV(),
					ctx.meshIndices->GetSRV()
				};
				ctx.context->CSSetShaderResources(0, 2, srvs);
			}
		}
		else if (m_spawnShape == 4) {
			// Texture Spawn: ParticleEmitter의 Baked Position 데이터 사용
			if (ctx.bakedSpawnPos) {
				ID3D11ShaderResourceView* srv[] = { ctx.bakedSpawnPos->GetSRV() };
				ctx.context->CSSetShaderResources(2, 1, srv);
			}
		}

		// ComputeCommon의 공유 ComputePSO 사용
		auto& spawnCS = RenderBase::computeCommon.particle.spawnCS;
		ctx.context->CSSetShader(spawnCS.computeShader.Get(), 0, 0);
		UINT groupCount = (m_totalSpawnCount + 1023) / 1024;
		ctx.context->Dispatch(groupCount, 1, 1);
		
		// Barrier
		ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
		ID3D11UnorderedAccessView* nullUAVs[1] = { nullptr };
		ctx.context->CSSetShaderResources(0, 2, nullSRVs);
		ctx.context->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);
		ctx.context->CSSetShader(nullptr, 0, 0);
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
		}
		if (data.contains("spawnRate")) m_spawnRate = data["spawnRate"];
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

		return cloned;
	}
}