#include "pch.h"
#include "ParticleEmitter.h"
#include <random>
#include "EmitterModule.h"

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
		InitializeBuffers(device);

		m_consts.Initialize();

		for (auto& mod : m_modules)
			mod->Initialize(device.Get(), this);

		for (auto& mod : m_modules)
			mod->OnSpawn(context.Get());
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

		// 1. 생성 (좁은 원형 바닥에서 생성)
		config.spawnVolume = Vector3(0.5f, 0.0f, 0.5f); // Y축 0 (바닥)
		config.lifeRange = Vector2(0.5f, 1.2f); // 짧게 생존

		// 2. 물리 (위로 상승)
		// 중요: 불은 중력을 거슬러 올라갑니다. Gravity를 양수(+) Y로 설정하세요.
		config.gravity = Vector3(0.0f, 3.0f, 0.0f); // 부력 (Buoyancy)
		config.velocity = Vector3(0.0f, 1.0f, 0.0f); // 초기 상승 속도
		config.speedRange = Vector2(0.5f, 1.5f);
		config.randomDir = Vector3(0.2f, 0.1f, 0.2f); // 옆으로 살짝 퍼짐
		config.drag = 0.5f; // 공기 저항 (끝에서 느려짐)

		// 3. 시각 (색상 및 크기)
		// 시작: 밝은 노랑/주황 -> 끝: 어두운 빨강/투명
		config.startColor = Vector4(1.0f, 0.8f, 0.2f, 1.0f);
		config.endColor = Vector4(0.8f, 0.1f, 0.0f, 0.0f); // Alpha 0으로 사라짐

		// 크기: 중간 크기 -> 작아짐 (불꽃 끝부분 수축)
		config.sizeRange = Vector2(1.5f, 0.5f);
	}

	void ParticleEmitter::InitializeShaders(ID3D11Device* device)
	{
		// 셰이더 로드
		m_spawnCS.Initialize(device, L"SpawnCS.hlsl");
		m_argsUpdateCS.Initialize(device, L"ParticleArgsUpdateCS.hlsl");
		m_particleCS.Initialize(device, L"ParticleCS.hlsl");
		m_InitSortKeysCS.Initialize(device, L"InitBitonicSortCS.hlsl");
	}

	void ParticleEmitter::InitializeBuffers(ComPtr<ID3D11Device>& device)
	{
		// 핑퐁 업데이트를 위한 이중 버퍼 파티클 저장소
		m_consume.Initialize(device.Get(), maxParticles);
		m_append.Initialize(device.Get(), maxParticles);

		// 간접 디스패치 및 드로우 인수
		m_dispatchArgs.Initialize(device.Get(), { 0, 1, 1 });
		m_drawInstancedArgs.Initialize(device.Get(), { 0, 1, 0, 0 });

		// 활성 파티클 개수를 추적하는 카운터 버퍼
		D3D11Utils::CreateBuffer(device.Get(), sizeof(UINT), nullptr,
			DXGI_FORMAT_R32_UINT, m_countBuffer, m_countSRV);

		m_sort.Initialize(device.Get(), maxParticles, L"BitonicSortCS.hlsl");
	}

	void ParticleEmitter::Update(const float& dt)
	{
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		m_time += dt;
		m_consts.GetCpu().dt = dt;
		m_consts.GetCpu().time = m_time;

		for (auto& mod : m_modules)
			mod->PreUpdate(context.Get(), dt);

		//m_elapsedTime += dt;
		//m_spawnAccumulator += m_targetSpawnRate * dt;

		//// 상수 버퍼 업데이트
		//m_consts.GetCpu().dt = dt;
		//m_consts.GetCpu().time = m_elapsedTime;
		//m_consts.GetCpu().maxParticles = maxParticles;
		m_consts.Upload();

		//EmitterModule* spawnModule = GetModule<EmitterModule>();
		//if (spawnModule && !spawnModule->CanSpawn())
		//	return;

		for (auto& mod : m_modules)
			mod->OnUpdate(context.Get(), dt);

		// GPU 파티클 업데이트 파이프라인 실행
		UpdateSpawnStage(context.Get(), dt);
		UpdateArgsBuffers(context.Get());
		UpdateSimulationStage(context.Get());
	}

	void ParticleEmitter::UpdateSpawnStage(ID3D11DeviceContext* context, float dt)
	{
		//// 생성할 파티클 개수 계산
		//int spawnCycles = static_cast<int>(m_spawnAccumulator);

		//// 수동으로 요청은 Burst로 1번만 실행
		//int manualBurstCount = m_burstCount;
		//m_burstCount = 0;

		//int totalSpawnCount = (spawnCycles * m_particlePerSpawn) + manualBurstCount;

		//if (spawnCycles > 0)
		//	m_spawnAccumulator -= static_cast<float>(spawnCycles);

		//if (totalSpawnCount <= 0)
		//	return;
	

		//// 상수 버퍼에 스폰 개수 업데이트
		//m_consts.GetCpu().spawnCount = totalSpawnCount;
		m_consts.Upload();

		m_spawnCS.UpdateConsts(context, 0, 1, m_consts.GetAddressOf());

		ID3D11UnorderedAccessView* uav = m_consume.GetUAV();
		context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);

		// Spawn Compute Shader
		UINT groupCount = (m_consts.GetCpu().spawnCount + 255) / 256;
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

		SortParticles(context);

		for (auto& mod : m_modules)
			mod->OnRender(context.Get());

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

	void ParticleEmitter::SortParticles(Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context)
	{
		context->CSSetUnorderedAccessViews(0, 1, m_sort.m_array.GetAddressOfUAV(), nullptr);
		ID3D11ShaderResourceView* srvs[] = {
			m_append.GetSRV(),
			m_countSRV.Get()
		};
		context->CSSetShaderResources(0, 2, srvs);
		m_InitSortKeysCS.Dispatch(context.Get(), (maxParticles + 1023) / 1024, 1, 1);

		m_sort.Sort(context.Get());
	}

}