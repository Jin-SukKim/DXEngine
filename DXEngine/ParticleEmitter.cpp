#include "pch.h"
#include "ParticleEmitter.h"
#include <random>

namespace DE {
	ParticleEmitter::ParticleEmitter(const std::wstring& name) : Actor(name)
	{
	}

	void ParticleEmitter::Initialize()
	{
		ComPtr<ID3D11Device>& device = GET_SINGLE(RenderBase)->GetDevice();
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		// 1. 셰이더 먼저 로드
		m_spawnCS.Initialize(device.Get(), L"SpawnCS.hlsl");
		m_argsUpdateCS.Initialize(device.Get(), L"ParticleArgsUpdateCS.hlsl");
		m_particleCS.Initialize(device.Get(), L"ParticleCS.hlsl");

		// 2. 버퍼 생성 (초기값 없이 생성)
		m_consume.Initialize(device.Get(), maxParticles);
		m_append.Initialize(device.Get(), m_consume.Size()); // 크기는 같게

		m_dispatchArgs.Initialize(device.Get(), { 0, 1, 1 });
		m_drawInstancedArgs.Initialize(device.Get(), { 0, 1, 0, 0 });

		// 카운트 버퍼 (SRV 플래그 포함된 CreateBuffer 사용 필수)
		D3D11Utils::CreateBuffer(device.Get(), sizeof(UINT), 0, DXGI_FORMAT_R32_UINT, m_countBuffer, m_countSRV);

		m_consts.Initialize();

		// 3. 초기 카운트 0으로 리셋 (중요)
		// GPU 스폰 방식을 쓰므로 처음엔 비어있어야 합니다.
		ID3D11UnorderedAccessView* uav = m_consume.GetUAV();
		UINT initCount = 0;
		context->CSSetUnorderedAccessViews(0, 1, &uav, &initCount);

		// 해제
		ID3D11UnorderedAccessView* nullUAV = nullptr;
		context->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
	}

	void ParticleEmitter::Update(const float& dt)
	{
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		m_elapsedTime += dt;

		// GPU에 필요한 정보만 전달
		m_consts.GetCpu().dt = dt;
		m_consts.GetCpu().time = m_elapsedTime;
		m_consts.GetCpu().spawnRate = m_targetSpawnRate;
		m_consts.GetCpu().maxParticles = maxParticles;
		m_consts.Upload();

		// ----------------------------------------------------
		// 1. Spawn 단계 항상 실행 (GPU가 생성 여부 결정)
		// ----------------------------------------------------
		{
			// activeCount를 t0에 바인딩
			context->CSSetShaderResources(0, 1, m_countSRV.GetAddressOf());

			ID3D11UnorderedAccessView* uav = m_consume.GetUAV();
			context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);

			// 충분한 스레드 그룹 Dispatch (GPU가 필터링)
			UINT maxPossibleSpawn = static_cast<UINT>(m_targetSpawnRate * dt * 2.0f);
			UINT dispatchGroups = std::max(1u, (maxPossibleSpawn + 255) / 256);

			m_spawnCS.UpdateConsts(context.Get(), 0, 1, m_consts.GetAddressOf());
			m_spawnCS.Dispatch(context.Get(), dispatchGroups, 1, 1);

			// 해제
			ID3D11ShaderResourceView* nullSRV = nullptr;
			context->CSSetShaderResources(0, 1, &nullSRV);
			ID3D11UnorderedAccessView* nullUAV = nullptr;
			context->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
		}

		// ----------------------------------------------------
		// 2. Counter 복사 및 Args 업데이트
		// ----------------------------------------------------
		context->CopyStructureCount(m_countBuffer.Get(), 0, m_consume.GetUAV());

		ID3D11UnorderedAccessView* argUAVs[] = {
			m_dispatchArgs.GetUAV(),
			m_drawInstancedArgs.GetUAV(),
		};

		context->CSSetShaderResources(0, 1, m_countSRV.GetAddressOf());
		context->CSSetUnorderedAccessViews(0, 2, argUAVs, nullptr);

		m_argsUpdateCS.Dispatch(context.Get(), 1, 1, 1);

		// 해제
		ID3D11ShaderResourceView* nullSRV = nullptr;
		context->CSSetShaderResources(0, 1, &nullSRV);
		ID3D11UnorderedAccessView* nullUAVs[] = { nullptr, nullptr };
		context->CSSetUnorderedAccessViews(0, 2, nullUAVs, nullptr);

		// ----------------------------------------------------
		// 3. Simulation 단계
		// ----------------------------------------------------
		context->CSSetShaderResources(0, 1, m_countSRV.GetAddressOf());

		UINT initCounts[2] = { (UINT)-1, 0 };

		ID3D11UnorderedAccessView* particleUAVs[] = {
			m_consume.GetUAV(),
			m_append.GetUAV()
		};

		context->CSSetUnorderedAccessViews(0, 2, particleUAVs, initCounts);
		m_particleCS.UpdateConsts(context.Get(), 0, 1, m_consts.GetAddressOf());
		m_particleCS.DispatchIndirect(context.Get(), m_dispatchArgs.GetBuffer());

		// 정리
		context->CSSetShaderResources(0, 1, &nullSRV);
		context->CSSetUnorderedAccessViews(0, 2, nullUAVs, nullptr);
	}

	void ParticleEmitter::Render()
	{
		RenderBase& renderer = *GET_SINGLE(RenderBase);
		ComPtr<ID3D11DeviceContext>& context = renderer.GetContext();

		renderer.SetPipelineState(RenderBase::graphicsCommon.particle.animPSO);
		context->VSSetShaderResources(0, 1, m_append.GetAddressOfSRV());
		context->DrawInstancedIndirect(m_drawInstancedArgs.GetBuffer(), 0);

		ID3D11ShaderResourceView* nullSRVs[1] = { NULL };
		context->VSSetShaderResources(0, 1, nullSRVs);
		swap(m_consume, m_append);
	}
}