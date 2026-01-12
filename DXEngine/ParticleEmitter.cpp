#include "pch.h"
#include "ParticleEmitter.h"
#include <random>
#include "RenderModule.h"

namespace DE {

	ParticleEmitter::ParticleEmitter(const std::wstring& name) 
		: Actor(name)
	{
	}

	void ParticleEmitter::Initialize()
	{
		ComPtr<ID3D11Device>& device = GET_SINGLE(RenderBase)->GetDevice();
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		for (auto& mod : m_modules)
			mod->Initialize(device.Get(), this);

		InitializeShaders(device.Get());
		InitializeBuffers(device);

		m_consts.Initialize();

		for (auto& mod : m_modules)
			mod->OnSpawn(context.Get());
	}

	void ParticleEmitter::SetParticleConfig(const ParticleConsts& config)
	{
		m_consts.SetCpuData(config);
	}

	void ParticleEmitter::InitializeShaders(ID3D11Device* device)
	{
		// 셰이더 로드
		m_argsUpdateCS.Initialize(device, L"ParticleArgsUpdateCS.hlsl");
		m_particleCS.Initialize(device, L"ParticleCS.hlsl");
	}

	void ParticleEmitter::InitializeBuffers(ComPtr<ID3D11Device>& device)
	{
		// 핑퐁 업데이트를 위한 이중 버퍼 파티클 저장소
		m_consume.Initialize(device.Get(), m_consts.GetCpu().maxParticles);
		m_append.Initialize(device.Get(), m_consts.GetCpu().maxParticles);

		// 간접 디스패치 및 드로우 인수
		m_dispatchArgs.Initialize(device.Get(), { 0, 1, 1 });
		m_drawInstancedArgs.Initialize(device.Get(), { 0, 1, 0, 0 });

		// 활성 파티클 개수를 추적하는 카운터 버퍼
		D3D11Utils::CreateBuffer(device.Get(), sizeof(UINT), nullptr,
			DXGI_FORMAT_R32_UINT, m_countBuffer, m_countSRV);

		
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
		UpdateArgsBuffers(context.Get());
		UpdateSimulationStage(context.Get());
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

		for (auto& mod : m_modules)
			mod->OnRender(context.Get());

		if (auto* renderMod = GetModule<RenderModule>()) {
			renderMod->Draw(context.Get(), m_drawInstancedArgs.GetBuffer(), m_append.GetSRV(), m_countSRV.Get());
		}

		// 다음 프레임을 위한 버퍼 교환 
		swap(m_consume, m_append);
	}
}