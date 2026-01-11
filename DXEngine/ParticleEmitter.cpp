#include "pch.h"
#include "ParticleEmitter.h"
#include <random>

namespace DE {

	ParticleEmitter::ParticleEmitter(const std::wstring& name) 
		: Actor(name)
	{
	}

	void ParticleEmitter::Initialize()
	{
		ComPtr<ID3D11Device>& device = GET_SINGLE(RenderBase)->GetDevice();
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		InitializeShaders(device.Get());
		InitializeBuffers(device.Get());

		m_InitSortKeysCS.Initialize(device.Get(), L"InitBitonicSortCS.hlsl");
		m_sort.Initialize(device, maxParticles, L"BitonicSortCS.hlsl");

		m_consts.Initialize();
	}

	void ParticleEmitter::SetBurst(UINT count)
	{
		m_burstCount = count;
	}

	void ParticleEmitter::SetParticlesPerSpawn(UINT count)
	{
		m_particlePerSpawn = count;
	}

	void ParticleEmitter::SetParticleConfig(const ParticleConsts& config)
	{
		m_consts.SetCpuData(config);
	}

	void ParticleEmitter::SetupFire()
	{
		ParticleConsts config;
		config.spawnVolume = Vector3(0.5f, 0.7f, 0.5f) * 0.2f; // 좁은 바닥 영역
		config.velocityBase = { 0.0f, 1.f, 0.0f }; // 위로 상승
		config.velocityRand = { 0.2f, 0.5f, 0.2f };  // 약간 흔들림
		config.velocity = 0.01f;

		config.lifeTimeBase = 1.0f;
		config.lifeTimeRand = 0.3f;
		config.minMaxRotateSpeed = Vector2(1.f, 3.f);

		config.gravity = { 0.0f, 1.0f, 0.0f };      // 부력 (위로 가속)
		config.drag = 0.0f;                         // 저항 없음

		config.minMaxSize = Vector2(0.25f, 0.05f);
		config.startColor = { 1.0f, 0.1f, 0.0f };  // 빨강
		config.endColor = { 1.0f, 0.8f, 0.1f };   // 노랑 

		SetSpawnRate(50.f);
		SetParticlesPerSpawn(10);
		SetParticleConfig(config);
		SetBlendMode(BlendMode::Additive);
	}

	void ParticleEmitter::SetupExplosion()
	{
		ParticleConsts config;
		config.spawnVolume = { 0.1f, 0.1f, 0.1f };  // 한 점(작은 구)에서 시작
		config.velocityBase = { 0.0f, 0.0f, 0.0f }; // 방향성 없음
		config.velocityRand = { 1.0f, 1.0f, 1.0f };  // 사방으로 퍼짐
		config.velocity = 10.0f;               // 매우 빠른 초기 속도!

		config.gravity = { 0.0f, 0.0f, 0.0f };     // 약한 중력
		config.drag = 20.0f;                         // 핵심: 강한 저항 (팡! 터지고 금방 느려짐)

		config.lifeTimeBase = 0.2f;
		config.lifeTimeRand = 0.5f;
		config.minMaxRotateSpeed = Vector2(1.f, 3.f);

		config.minMaxSize = Vector2(0.02f, 0.15f);
		config.startColor = { 1.0f, 0.0f, 0.0f };   // 흰색 섬광
		config.endColor = { 0.0f, 0.0f, 0.0f };     // 검은 연기

		SetSpawnRate(1.f);
		SetParticlesPerSpawn(200);
		SetParticleConfig(config);

		SetBlendMode(BlendMode::AlphaBlend);
	}

	void ParticleEmitter::SetBlendMode(BlendMode mode)
	{
		m_blendMode = mode;

		switch (m_blendMode)
		{
		case BlendMode::Additive:
			RenderBase::graphicsCommon.particle.animPSO.blendState = RenderBase::graphicsCommon.accumulateBS;
			break;
		case BlendMode::AlphaBlend:
			RenderBase::graphicsCommon.particle.animPSO.blendState = RenderBase::graphicsCommon.alphaBS;
			break;
		case BlendMode::Opaque:
			RenderBase::graphicsCommon.particle.animPSO.blendState = nullptr;
			break;
		}
	}

	void ParticleEmitter::InitializeShaders(ID3D11Device* device)
	{
		// 셰이더 로드
		m_spawnCS.Initialize(device, L"SpawnCS.hlsl");
		m_argsUpdateCS.Initialize(device, L"ParticleArgsUpdateCS.hlsl");
		m_particleCS.Initialize(device, L"ParticleCS.hlsl");
	}

	void ParticleEmitter::InitializeBuffers(ID3D11Device* device)
	{
		// 핑퐁 업데이트를 위한 이중 버퍼 파티클 저장소
		m_consume.Initialize(device, maxParticles);
		m_append.Initialize(device, maxParticles);

		// 간접 디스패치 및 드로우 인수
		m_dispatchArgs.Initialize(device, { 0, 1, 1 });
		m_drawInstancedArgs.Initialize(device, { 0, 1, 0, 0 });

		// 활성 파티클 개수를 추적하는 카운터 버퍼
		D3D11Utils::CreateBuffer(device, sizeof(UINT), nullptr, 
			DXGI_FORMAT_R32_UINT, m_countBuffer, m_countSRV);
	}

	void ParticleEmitter::Update(const float& dt)
	{
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		m_elapsedTime += dt;
		m_spawnAccumulator += m_targetSpawnRate * dt;

		// 상수 버퍼 업데이트
		m_consts.GetCpu().dt = dt;
		m_consts.GetCpu().time = m_elapsedTime;
		m_consts.GetCpu().maxParticles = maxParticles;
		m_consts.Upload();

		// GPU 파티클 업데이트 파이프라인 실행
		UpdateSpawnStage(context.Get(), dt);
		UpdateArgsBuffers(context.Get());
		UpdateSimulationStage(context.Get());
	}

	void ParticleEmitter::UpdateSpawnStage(ID3D11DeviceContext* context, float dt)
	{
		// 생성할 파티클 개수 계산
		int spawnCycles = static_cast<int>(m_spawnAccumulator);

		// 수동으로 요청은 Burst로 1번만 실행
		int manualBurstCount = m_burstCount;
		m_burstCount = 0;

		int totalSpawnCount = (spawnCycles * m_particlePerSpawn) + manualBurstCount;

		if (spawnCycles > 0)
			m_spawnAccumulator -= static_cast<float>(spawnCycles);

		if (totalSpawnCount <= 0)
			return;
	

		// 상수 버퍼에 스폰 개수 업데이트
		m_consts.GetCpu().spawnCount = totalSpawnCount;
		m_consts.Upload();

		m_spawnCS.UpdateConsts(context, 0, 1, m_consts.GetAddressOf());
		context->CSSetShaderResources(0, 1, m_countSRV.GetAddressOf());

		ID3D11UnorderedAccessView* uav = m_consume.GetUAV();
		context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);

		// Spawn Compute Shader
		UINT groupCount = (totalSpawnCount + 255) / 256;
		m_spawnCS.Dispatch(context, groupCount, 1, 1);
	}

	void ParticleEmitter::UpdateArgsBuffers(ID3D11DeviceContext* context)
	{
		// Append 버퍼에서 현재 파티클 개수를 카운트 버퍼로 복사
		context->CopyStructureCount(m_countBuffer.Get(), 0, m_consume.GetUAV());

		ID3D11UnorderedAccessView* argUAVs[] = {
			m_dispatchArgs.GetUAV(),
			m_drawInstancedArgs.GetUAV()
		};

		context->CSSetShaderResources(0, 1, m_countSRV.GetAddressOf());
		context->CSSetUnorderedAccessViews(0, 2, argUAVs, nullptr);

		// Indirect Args Update
		m_argsUpdateCS.Dispatch(context, 1, 1, 1);
	}

	void ParticleEmitter::UpdateSimulationStage(ID3D11DeviceContext* context)
	{
		// Counter buffer binding
		context->CSSetShaderResources(0, 1, m_countSRV.GetAddressOf());

		// UAV 설정 (초기 카운트 지정)
		// -1: consume 버퍼의 기존 카운트 유지
		// 0: append 버퍼의 카운트 리셋
		UINT initCounts[2] = { static_cast<UINT>(-1), 0 };
		ID3D11UnorderedAccessView* particleUAVs[] = {
			m_consume.GetUAV(),
			m_append.GetUAV()
		};

		context->CSSetUnorderedAccessViews(0, 2, particleUAVs, initCounts);
		m_particleCS.UpdateConsts(context, 0, 1, m_consts.GetAddressOf());

		// Particle Simulation Compute Shader
		m_particleCS.DispatchIndirect(context, m_dispatchArgs.GetBuffer());
	}

	void ParticleEmitter::Render(const ComPtr<ID3D11Buffer>& globalConstsGPU)
	{
		RenderBase& renderer = *GET_SINGLE(RenderBase);
		ComPtr<ID3D11DeviceContext>& context = renderer.GetContext();

		context->CSSetUnorderedAccessViews(0, 1, m_sort.m_array.GetAddressOfUAV(), nullptr);
		ID3D11ShaderResourceView* srvs[] = {
			m_append.GetSRV(),
			m_countSRV.Get()
		};
		context->CSSetShaderResources(0, 2, srvs);
		m_InitSortKeysCS.Dispatch(context.Get(), (maxParticles + 1023) / 1024, 1, 1);

		m_sort.Sort(GET_SINGLE(RenderBase)->GetDevice().Get(), context.Get());

		renderer.SetPipelineState(RenderBase::graphicsCommon.particle.animPSO);

		// IndirectDraw
		ID3D11ShaderResourceView* sortSRVs[] = {
			m_append.GetSRV(),
			m_sort.m_array.GetSRV()
		};
		context->VSSetShaderResources(0, 2, sortSRVs);
		context->DrawInstancedIndirect(m_drawInstancedArgs.GetBuffer(), 0);

		// 정리
		ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
		context->VSSetShaderResources(0, 2, nullSRVs);

		// 다음 프레임을 위한 버퍼 교환 
		swap(m_consume, m_append);
	}
}