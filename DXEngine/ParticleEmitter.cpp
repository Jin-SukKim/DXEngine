#include "pch.h"
#include "ParticleEmitter.h"
#include <random>

namespace DE {

	UINT particleCount = 1024;

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
		m_consume.Initialize(device.Get(), particleCount);
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

		// 상수 버퍼 업데이트
		m_consts.GetCpu().dt = dt * 0.5f;
		m_consts.Upload();
		m_particleCS.UpdateConsts(context.Get(), 0, 1, m_consts.GetAddressOf());

		// ----------------------------------------------------
		// 1. Spawn 단계 (매 프레임 10개 추가)
		// ----------------------------------------------------
		{
			// m_consume 버퍼에 Append (마지막 인자 nullptr = 카운트 유지)
			ID3D11UnorderedAccessView* uav = m_consume.GetUAV();
			context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);

			// 10개 생성 (SpawnCS에서 스레드 10개라고 가정)
			m_spawnCS.Dispatch(context.Get(), 1, 1, 1);

			// [중요] UAV 해제 (다음 단계에서 읽기/쓰기 충돌 방지)
			ID3D11UnorderedAccessView* nullUAV = nullptr;
			context->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
		}

		// ----------------------------------------------------
		// 2. Counter 복사 및 Args 업데이트
		// ----------------------------------------------------
		// 방금 10개 추가된 카운트를 복사
		context->CopyStructureCount(m_countBuffer.Get(), 0, m_consume.GetUAV());

		ID3D11UnorderedAccessView* argUAVs[] = {
			m_dispatchArgs.GetUAV(),
			m_drawInstancedArgs.GetUAV(),
		};

		// Count(SRV) 읽어서 Args(UAV) 씀
		context->CSSetShaderResources(0, 1, m_countSRV.GetAddressOf());
		context->CSSetUnorderedAccessViews(0, 2, argUAVs, nullptr);

		m_argsUpdateCS.Dispatch(context.Get(), 1, 1, 1);

		// 해제 (필수)
		ID3D11ShaderResourceView* nullSRV = nullptr;
		context->CSSetShaderResources(0, 1, &nullSRV);
		ID3D11UnorderedAccessView* nullUAVs[] = { nullptr, nullptr };
		context->CSSetUnorderedAccessViews(0, 2, nullUAVs, nullptr);

		// ----------------------------------------------------
		// 3. Simulation 단계
		// ----------------------------------------------------
		context->CSSetShaderResources(0, 1, m_countSRV.GetAddressOf()); // t0

		// [수정 핵심 1] m_consume(Input)은 현재 카운트 유지(-1), m_append(Output)는 0으로 리셋
		UINT initCounts[2] = { (UINT)-1, 0 };

		ID3D11UnorderedAccessView* particleUAVs[] = {
			m_consume.GetUAV(),
			m_append.GetUAV()
		};

		// [수정 핵심 2] StartSlot을 2가 아니라 0으로 변경 (u0, u1에 연결)
		context->CSSetUnorderedAccessViews(0, 2, particleUAVs, initCounts);

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

	void ParticleEmitter::GenerateRandomParticles(StructuredBuffer<Particle>& particles)
	{
		std::vector<Particle> randomParticles;
		randomParticles.reserve(particleCount);

		std::vector<Vector3> colors = {
			{1.f, 0.f, 0.f}, // Red
			{1.f, 0.65f, 0.f}, // orange
			{1.f, 1.f, 0.f}, // Yellow
			{0.f, 1.f, 0.f}, // Green
			{0.f, 0.f, 1.f}, // Blue
			{1.f, 0.f, 1.f}, // Purple
			{1.f, 1.f, 1.f}, // White
		};

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> df(-1.f, 1.f);
		std::uniform_real_distribution<float> randomSpeed(1.f, 2.f);
		std::uniform_real_distribution<float> randomLife(0.f, 1.f);
		std::uniform_int_distribution<UINT> di(0, static_cast<UINT>(colors.size() - 1));

		for (UINT i = 0; i < particleCount; ++i) {
			Particle p;
			p.position = Vector3(df(gen), df(gen), 0.f);
			p.color = colors[di(gen)];
			p.size = (df(gen)) * 0.02f;
			p.velocity = Vector3(df(gen), 1.f, df(gen)) * randomSpeed(gen);
			p.life = 1.f;

			randomParticles.emplace_back(p);
		}

		particles.SetData(randomParticles);
	}

}