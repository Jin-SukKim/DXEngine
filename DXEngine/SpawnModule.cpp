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
		SpawnConsts& consts = ctx.constsData.spawn;
		consts.localPos = m_localPos;
		consts.spawnVolume = m_spawnVolume;
		consts.spawnInnerRatio = m_spawnInnerRatio;
		consts.spawnShape = m_spawnShape;
		consts.lifeRange = m_lifeRange;
		consts.bakedCount = ctx.bakedCount;
		consts.simulationSpace = m_simulationSpace;
		m_burstFired = false;
		m_spawnAccumulator = 0.0f;

		if (m_spawnShape == 5) // Custom Mode
		{
			UINT posCount = (UINT)m_customPositions.size();
			ctx.customPositions->Initialize(ctx.device, posCount);
			ctx.customPositions->SetData(m_customPositions);
			ctx.customPositions->Upload(ctx.context);

			consts.bakedCount = posCount;

			// 1. 이번 프레임의 시작 인덱스를 GPU에 전달
			consts.spawnStartIndex = m_nextSpawnIndex;

			// 2. 다음 프레임을 위해 인덱스 미리 이동 (Round-Robin)
			// m_totalSpawnCount는 이번 프레임에 생성될 총 파티클 수입니다.
			if (posCount > 0)
			{
				m_nextSpawnIndex = (m_nextSpawnIndex + m_totalSpawnCount) % posCount;
			}
		}

		ctx.frameConstBuffer.GetCpu().maxParticles = m_maxParticles;
	}

	void SpawnModule::OnPreUpdate(const SimulationContext& ctx)
	{
		ParticleModule::OnPreUpdate(ctx);
		// Burst 로직: 아직 발사 안 했고, 설정된 Burst 개수가 있다면
		if (m_burstCount > 0 && !m_burstFired)
		{
			m_spawnAccumulator += (float)m_burstCount;
			m_burstFired = true; // 발사 완료 처리
		}

		// Rate 로직 (지속 생성)
		if (m_spawnRate > 0.0f)
		{
			m_spawnAccumulator += m_spawnRate * ctx.dt;
		}

		//m_totalSpawnCount = 1;
		UINT spawnCycles = static_cast<int>(m_spawnAccumulator);
		m_totalSpawnCount = spawnCycles * m_particlesPerSpawn;

		if (spawnCycles > 0)
			m_spawnAccumulator -= static_cast<float>(spawnCycles);
		if (m_totalSpawnCount < 0)
			m_totalSpawnCount = 0;

		ctx.frameConstBuffer.GetCpu().spawnCount = m_totalSpawnCount;
	}

	void SpawnModule::LateUpdate(SimulationContext& ctx)
	{
		ParticleModule::LateUpdate(ctx);
		
		// [최적화] 조기 반환
		if (m_totalSpawnCount == 0)
			return;

		ID3D11UnorderedAccessView* uavs[2] = {
			ctx.writeParticles.GetUAV(),
			ctx.writeCount.GetUAV()
		};
		ctx.context->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);

		// [최적화] Shape별 SRV 바인딩 최적화
		if (m_spawnShape == 2 || m_spawnShape == 3) {

		}
		else if (m_spawnShape == 4) {
			if (ctx.bakedSpawnPos) {
				ID3D11ShaderResourceView* srv = ctx.bakedSpawnPos->GetSRV();
				ctx.context->CSSetShaderResources(0, 1, &srv);
			}
		}
		else if (m_spawnShape == 5) {
			if (ctx.customPositions) {
				ID3D11ShaderResourceView* srv = ctx.customPositions->GetSRV();
				ctx.context->CSSetShaderResources(0, 1, &srv);
			}
		}

		auto& spawnCS = RenderBase::computeCommon.particle.spawnCS;
		ctx.context->CSSetShader(spawnCS.computeShader.Get(), 0, 0);
		
		// [최적화] Bit shift 사용
		UINT groupCount = (m_totalSpawnCount + 1023) >> 10;
		ctx.context->Dispatch(groupCount, 1, 1);
		
		// Barrier
		ID3D11ShaderResourceView* nullSRVs[3] = { nullptr };
		ID3D11UnorderedAccessView* nullUAVs[2] = { nullptr, nullptr };
		ctx.context->CSSetShaderResources(0, 3, nullSRVs);
		ctx.context->CSSetUnorderedAccessViews(0, 2, nullUAVs, nullptr);
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
			else if (shape == "Custom") {
				if (data.contains("positions") && data["positions"].is_array()) {
					std::vector<Vector3> positions;
					// json 배열을 순회하며 Vector3로 변환하여 저장
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
		// 1. 위치 데이터 저장
		m_customPositions = positions;

		// 2. 모드를 Custom(5)으로 설정
		m_spawnShape = 5;

		// 3. 순차적 인덱스 초기화 (새로운 위치 목록이 들어왔으므로 처음부터 다시 시작)
		m_nextSpawnIndex = 0;
	}
}