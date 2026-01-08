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

	GenerateRandomParticles(m_consume);
	m_consume.Initialize(device.Get());

	m_append.Initialize(device.Get(), m_consume.Size());

	m_dispatchArgs.Initialize(device.Get(), { 0, 1, 1 });
	m_drawInstancedArgs.Initialize(device.Get(), { 0, 1, 0, 0 });
	D3D11Utils::CreateBuffer(device.Get(), sizeof(UINT), 0, DXGI_FORMAT_R32_UINT, m_countBuffer, m_countSRV);

	m_argsUpdateCS.Initialize(device.Get(), L"ParticleArgsUpdateCS.hlsl");
	m_particleCS.Initialize(device.Get(), L"ParticleCS.hlsl");

	m_consts.Initialize();

	// [추가] 초기 파티클 개수만큼 m_consume의 카운터를 설정해줍니다.
	// 이렇게 해야 첫 프레임의 CopyStructureCount가 올바른 개수(1024)를 가져옵니다.
	ID3D11UnorderedAccessView* uav = m_consume.GetUAV();
	UINT initCount = m_consume.Size(); // 초기 개수 (1024)
	context->CSSetUnorderedAccessViews(0, 1, &uav, &initCount);

	// 설정 후 바로 해제 (다른 곳에 영향을 주지 않기 위해)
	ID3D11UnorderedAccessView* nullUAV = nullptr;
	context->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
}

void ParticleEmitter::Update(const float& dt)
{
	ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();
	m_consts.GetCpu().dt = dt * 0.5f;
	m_consts.Upload();

	m_particleCS.UpdateConsts(context.Get(), 0, 1, m_consts.GetAddressOf());

	context->CopyStructureCount(m_countBuffer.Get(), 0, m_consume.GetUAV());

	ID3D11UnorderedAccessView* argUAVs[] = {
		m_dispatchArgs.GetUAV(),    
		m_drawInstancedArgs.GetUAV(), 
	};
	context->CSSetShaderResources(0, 1, m_countSRV.GetAddressOf());
	context->CSSetUnorderedAccessViews(0, 2, argUAVs, nullptr);
	m_argsUpdateCS.Dispatch(context.Get(), 1, 1, 1);

	context->CSSetShaderResources(0, 1, m_countSRV.GetAddressOf());
	UINT initCounts[2] = { static_cast<UINT>(m_consume.Size()), 0 };
	ID3D11UnorderedAccessView* particleUAVs[] = {
		m_consume.GetUAV(),
		m_append.GetUAV()
	};
	context->CSSetUnorderedAccessViews(2, 2, particleUAVs, initCounts);
	m_particleCS.DispatchIndirect(context.Get(), m_dispatchArgs.GetBuffer());
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

	// 임시로 사용할 색상
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
		//p.life = randomLife(gen);
		p.life = 1.f;

		randomParticles.emplace_back(p);
	}

	particles.SetData(randomParticles);
}

}
