#include "pch.h"
#include "ParticleEmitter.h"
#include <random>

namespace DE {

	UINT particleCount = 500;
ParticleEmitter::ParticleEmitter(const std::wstring& name) : Actor(name)
{
}

void ParticleEmitter::Initialize()
{
	ComPtr<ID3D11Device>& device = GET_SINGLE(RenderBase)->GetDevice();
	ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();
	//m_particles.Initialize(device.Get(), particleCount);
	//GenerateRandomParticles(m_particles);
	//m_particles.Upload(context.Get());

	GenerateRandomParticles(m_consume);
	m_consume.Initialize(device.Get());

	m_append.Initialize(device.Get(), m_consume.Size());
	m_activeCount.Initialize(device.Get(), { 0 }); // Count값 1개

	m_particleCS.Initialize(device.Get(), L"ParticleCS.hlsl");

	m_consts.Initialize();
}

void ParticleEmitter::Update(const float& dt)
{
	ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();
	m_consts.GetCpu().dt = dt * 0.5f;
	m_consts.Upload();

	m_particleCS.UpdateConsts(context.Get(), 0, 1, m_consts.GetAddressOf());
}

void ParticleEmitter::Render()
{
	RenderBase& renderer = *GET_SINGLE(RenderBase);
	ComPtr<ID3D11DeviceContext>& context = renderer.GetContext();

	// 각 UAV의 시작 크기
	UINT initCounts[2] = { static_cast<UINT>(m_consume.Size()), 0 };
	m_particleCS.SetUAVs(context.Get(), 0, 
		{m_consume.GetUAV(), m_append.GetUAV()},
		initCounts);
	m_particleCS.Dispatch(context.Get(), UINT(ceil(m_consume.Size() / 1024.f)), 1, 1);

	// Append의 UAV 개수 복사
	context->CopyStructureCount(m_activeCount.GetGpu(), 0, m_append.GetUAV());
	m_activeCount.Download(context.Get());
	
	renderer.SetPipelineState(RenderBase::graphicsCommon.particle.animPSO);
	context->VSSetShaderResources(0, 1, m_append.GetAddressOfSRV());
	context->Draw(m_activeCount.GetCpu(0), 0); // append에 저장된 particle 개수
	
	//ID3D11ShaderResourceView* nullSRVs[1] = { NULL };
	//context->VSSetShaderResources(0, 1, nullSRVs);
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
