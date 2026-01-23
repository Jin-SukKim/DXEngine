#include "pch.h"
#include "SpawnModule.h"
#include "ParticleEmitter.h"
#include "Mesh.h"
#include "TextureSpawnBake.h"

namespace DE {
	void SpawnModule::Initialize(ParticleInitContext& ctx)
	{
		ctx.frameConsts.maxParticles = m_maxParticles;
		m_spawnCS.Initialize(ctx.device, L"SpawnCS.hlsl");
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
		consts.vertexCount = m_vertexCount;
		consts.indexCount = m_indexCount;
		consts.bakedCount = m_bakedCount;
		consts.simulationSpace = m_simulationSpace;

		ctx.frameConstBuffer.GetCpu().maxParticles = m_maxParticles;
	}

	void SpawnModule::OnUpdateCPU(SimulationContext& ctx)
	{
		ParticleModule::OnUpdateCPU(ctx);
		m_spawnAccumulator += m_spawnRate * ctx.dt;

		//m_totalSpawnCount = 1;
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
			ID3D11ShaderResourceView* srvs[] = {
				m_meshVertex.GetSRV(),
				m_meshIndices.GetSRV()
			};
			ctx.context->CSSetShaderResources(0, 2, srvs);
		}
		else if (m_spawnShape == 4) {
			ID3D11ShaderResourceView* srv[] = { m_spawnPos.GetSRV() };
			ctx.context->CSSetShaderResources(2, 1, srv);
		}

		// Spawn Compute Shader
		UINT groupCount = (m_totalSpawnCount + 255) / 256;
		m_spawnCS.Dispatch(ctx.context, groupCount, 1, 1);
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
			else if (shape == "Texture") {
				if (data.contains("bakedPath")) {
					m_spawnShape = 4;
					std::string path = data["bakedPath"];
					TextureSpawnBake::Get().LoadBakedData(path, m_spawnPos, m_bakedCount);
				}
				else
					m_spawnShape = 1;
			}
		}
		if (data.contains("spawnRate")) m_spawnRate = data["spawnRate"];
		if (data.contains("particlesPerSpawn")) m_particlesPerSpawn = data["particlesPerSpawn"];
		if (data.contains("maxParticles")) m_maxParticles = data["maxParticles"];
		if (data.contains("lifeRange")) m_lifeRange = JsonToVec2(data["lifeRange"]);
	}

	void SpawnModule::SetTarget(const MeshData& meshes)
	{
		ComPtr<ID3D11Device>& device = GET_SINGLE(RenderBase)->GetDevice();
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		m_vertexCount = static_cast<UINT>(meshes.vertices.size());
		m_indexCount = static_cast<UINT>(meshes.indices.size());

		m_meshVertex.Initialize(device.Get(), m_vertexCount);
		m_meshIndices.Initialize(device.Get(), m_indexCount);

		m_meshVertex.SetData(meshes.vertices);
		m_meshIndices.SetData(meshes.indices);

		m_meshVertex.Upload(context.Get());
		m_meshIndices.Upload(context.Get());
	}
	// SpawnModule.cpp (공유 방식 - 더 간단)
	std::unique_ptr<ParticleModule> SpawnModule::Clone() const
	{
		auto cloned = std::make_unique<SpawnModule>();

		// 기본 설정값 복사
		cloned->m_localPos = this->m_localPos;
		cloned->m_spawnVolume = this->m_spawnVolume;
		cloned->m_spawnInnerRatio = this->m_spawnInnerRatio;
		cloned->m_spawnShape = this->m_spawnShape;
		cloned->m_spawnRate = this->m_spawnRate;
		cloned->m_particlesPerSpawn = this->m_particlesPerSpawn;
		cloned->m_maxParticles = this->m_maxParticles;
		cloned->m_lifeRange = this->m_lifeRange;
		cloned->m_vertexCount = this->m_vertexCount;
		cloned->m_indexCount = this->m_indexCount;
		cloned->m_simulationSpace = this->m_simulationSpace;
		cloned->m_bakedCount = this->m_bakedCount;
		cloned->m_isEnabled = this->m_isEnabled;

		// StructuredBuffer 전체 복사 (ComPtr는 참조 카운트 증가)
		// CPU 데이터(m_cpu)도 복사되고, GPU 버퍼(m_gpu, m_srv)는 공유됨
		if (m_spawnShape == 2 || m_spawnShape == 3) {
			// StructuredBuffer의 기본 복사 생성자 사용
			cloned->m_meshVertex = this->m_meshVertex;
			cloned->m_meshIndices = this->m_meshIndices;
		}

		if (m_spawnShape == 4) {
			cloned->m_spawnPos = this->m_spawnPos;
		}

		return cloned;
	}
}