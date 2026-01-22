#include "pch.h"
#include "SpawnModule.h"
#include "ParticleEmitter.h"
#include "Mesh.h"
#include "TextureSpawnBake.h"

namespace DE {
	void SpawnModule::Initialize(ParticleInitContext& ctx)
	{
		ctx.frameConsts.maxParticles = maxParticles;
		m_spawnCS.Initialize(ctx.device, L"SpawnCS.hlsl");
	}

	void SpawnModule::OnSpawn(SimulationContext& ctx)
	{
		ParticleModule::OnSpawn(ctx);
		SpawnConsts& consts = ctx.constBuffer.GetCpu().spawn;
		consts.localPos = localPos;
		consts.spawnVolume = spawnVolume;
		consts.spawnInnerRatio = spawnInnerRatio;
		consts.spawnShape = spawnShape;
		consts.lifeRange = lifeRange;
		consts.vertexCount = vertexCount;
		consts.indexCount = indexCount;
		consts.bakedCount = m_bakedCount;

		ctx.frameConstBuffer.GetCpu().maxParticles = maxParticles;
	}

	void SpawnModule::OnUpdateCPU(SimulationContext& ctx)
	{
		ParticleModule::OnUpdateCPU(ctx);
		spawnAccumulator += spawnRate * ctx.dt;

		m_totalSpawnCount = 1;
		//UINT spawnCycles = static_cast<int>(spawnAccumulator);
		//m_totalSpawnCount = spawnCycles * particlesPerSpawn;

		//if (spawnCycles > 0)
		//	spawnAccumulator -= static_cast<float>(spawnCycles);
		//if (m_totalSpawnCount < 0)
		//	m_totalSpawnCount = 0;

		ctx.frameConstBuffer.GetCpu().spawnCount = m_totalSpawnCount;
	}

	void SpawnModule::PreUpdate(SimulationContext& ctx)
	{
		ParticleModule::PreUpdate(ctx);
		
		if (m_totalSpawnCount == 0)
			return;

		ID3D11UnorderedAccessView* uav = ctx.consumeBuffer.GetUAV();
		ctx.context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);

		if (spawnShape == 2 || spawnShape == 3) {
			ID3D11ShaderResourceView* srvs[] = {
				m_meshVertex.GetSRV(),
				m_meshIndices.GetSRV()
			};
			ctx.context->CSSetShaderResources(0, 2, srvs);
		}
		else if (spawnShape == 4) {
			ID3D11ShaderResourceView* srv[] = { m_spawnPos.GetSRV() };
			ctx.context->CSSetShaderResources(2, 1, srv);
		}

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
			if (shape == "Box") spawnShape = 0;
			else if (shape == "Sphere") spawnShape = 1;
			else if (shape == "Vertex") spawnShape = 2;
			else if (shape == "Surface") spawnShape = 3;
			else if (shape == "Texture") {
				if (data.contains("bakedPath")) {
					spawnShape = 4;
					std::string path = data["bakedPath"];
					TextureSpawnBake::Get().LoadBakedData(path, m_spawnPos, m_bakedCount);
				}
				else
					spawnShape = 1;
			}
		}
		if (data.contains("spawnRate")) spawnRate = data["spawnRate"];
		if (data.contains("particlesPerSpawn")) particlesPerSpawn = data["particlesPerSpawn"];
		if (data.contains("maxParticles")) maxParticles = data["maxParticles"];
		if (data.contains("lifeRange")) lifeRange = JsonToVec2(data["lifeRange"]);
	}

	void SpawnModule::SetTarget(const MeshData& meshes)
	{
		ComPtr<ID3D11Device>& device = GET_SINGLE(RenderBase)->GetDevice();
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		vertexCount = static_cast<UINT>(meshes.vertices.size());
		indexCount = static_cast<UINT>(meshes.indices.size());

		m_meshVertex.Initialize(device.Get(), vertexCount);
		m_meshIndices.Initialize(device.Get(), indexCount);

		m_meshVertex.SetData(meshes.vertices);
		m_meshIndices.SetData(meshes.indices);

		m_meshVertex.Upload(context.Get());
		m_meshIndices.Upload(context.Get());
	}
}