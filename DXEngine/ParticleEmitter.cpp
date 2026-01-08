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

		m_consts.Initialize();
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
		int spawnCount = static_cast<int>(m_spawnAccumulator);
		if (spawnCount <= 0)
			return;

		// 생성할 파티클 개수만큼 누적기에서 차감
		m_spawnAccumulator -= static_cast<float>(spawnCount);
		
		// 상수 버퍼에 스폰 개수 업데이트
		m_consts.GetCpu().spawnCount = spawnCount;
		m_consts.Upload();

		m_spawnCS.UpdateConsts(context, 0, 1, m_consts.GetAddressOf());
		context->CSSetShaderResources(0, 1, m_countSRV.GetAddressOf());

		ID3D11UnorderedAccessView* uav = m_consume.GetUAV();
		context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);

		// Spawn Compute Shader
		UINT groupCount = (spawnCount + 255) / 256;
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

	void ParticleEmitter::Render()
	{
		RenderBase& renderer = *GET_SINGLE(RenderBase);
		ComPtr<ID3D11DeviceContext>& context = renderer.GetContext();

		renderer.SetPipelineState(RenderBase::graphicsCommon.particle.animPSO);
		
		// IndirectDraw
		context->VSSetShaderResources(0, 1, m_append.GetAddressOfSRV());
		context->DrawInstancedIndirect(m_drawInstancedArgs.GetBuffer(), 0);

		// 정리
		ID3D11ShaderResourceView* nullSRVs[1] = { nullptr };
		context->VSSetShaderResources(0, 1, nullSRVs);

		// 다음 프레임을 위한 버퍼 교환 
		swap(m_consume, m_append);
	}
}