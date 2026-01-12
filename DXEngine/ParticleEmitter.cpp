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

		m_consts.Upload();

		for (auto& mod : m_modules)
			mod->OnUpdate(context.Get(), dt);

		// GPU 파티클 업데이트 파이프라인 실행
		UpdateSpawnStage(context.Get(), dt);
		UpdateArgsBuffers(context.Get());
		UpdateSimulationStage(context.Get());
	}

	void ParticleEmitter::UpdateSpawnStage(ID3D11DeviceContext* context, float dt)
	{
		if (m_consts.GetCpu().spawnCount == 0)
			return;

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