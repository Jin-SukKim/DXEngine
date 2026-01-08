#include "pch.h"
#include "ParticleEmitter.h"
#include <random>

namespace DE {

	UINT particleCount = 1000;
ParticleEmitter::ParticleEmitter(const std::wstring& name) : Actor(name)
{
}

void ParticleEmitter::Initialize()
{
	ComPtr<ID3D11Device>& device = GET_SINGLE(RenderBase)->GetDevice();
	m_particles.Initialize(device.Get(), particleCount);
	GenerateRandomParticles(m_particles);
	ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();
	m_particles.Upload(context.Get());

	m_particleCS.Initialize(L"ParticleCS.hlsl");

	m_consts.Initialize();
}

void ParticleEmitter::Update(const float& dt)
{
	m_consts.GetCpu().dt = dt * 0.5f;
	m_consts.Upload();

	m_particleCS.UpdateConsts(0, 1, m_consts.GetAddressOf());
}

void ParticleEmitter::Render()
{
	RenderBase& renderer = *GET_SINGLE(RenderBase);
	ComPtr<ID3D11DeviceContext>& context = renderer.GetContext();

	std::vector<ID3D11ShaderResourceView*> srvs = {
		m_particles.GetSRV()
	};
	std::vector<ID3D11UnorderedAccessView*> uavs = {
		m_particles.GetUAV()
	};
	m_particleCS.Dispatch(UINT(ceil(m_particles.Size() / 1024.f)), 1, 1, srvs, uavs);

	renderer.SetPipelineState(RenderBase::graphicsCommon.particle.animPSO);
	context->VSSetShaderResources(0, 1, m_particles.GetAddressOfSRV());
	context->Draw(m_particles.Size(), 0);
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

	for (int i = 0; i < particleCount; ++i) {
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
