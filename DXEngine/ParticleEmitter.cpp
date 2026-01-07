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
}

void ParticleEmitter::Update(const float& dt)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> randomPosition(-1.f, 1.f);
	std::uniform_real_distribution<float> randomSpeed(1.f, 2.f);
	std::uniform_real_distribution<float> randomLife(0.f, 1.f);

	int newCount = 10;
	for (UINT i = 0; i < particleCount; ++i) {
		Particle& p = m_particles.Get(i);
		if (p.life < 0.f && newCount > 0) {
			p.position = Vector3(randomPosition(gen), randomPosition(gen), 0.f);
			p.velocity = Vector3(randomPosition(gen), 1.f, randomPosition(gen)) * randomSpeed(gen);
			p.life = randomLife(gen);
			--newCount;
		}
	}

	for (UINT i = 0; i < particleCount; ++i) {
		Particle& p = m_particles.Get(i);
		if (p.life < 0.f)
			continue;
		
		p.position += p.velocity * dt;
		p.life -= dt;
	}

	ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();
	m_particles.Upload(context.Get());
}

void ParticleEmitter::Render()
{
	RenderBase& renderer = *GET_SINGLE(RenderBase);
	ComPtr<ID3D11DeviceContext> context = renderer.GetContext();

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
	std::uniform_int_distribution<UINT> di(0, static_cast<UINT>(colors.size() - 1));

	for (int i = 0; i < particleCount; ++i) {
		Particle p;
		p.position = Vector3(df(gen), df(gen), 0.f);
		p.color = colors[di(gen)];
		p.size = (df(gen) + 1.3f) * 0.5f;
		p.life = -1.f;

		randomParticles.emplace_back(p);
	}

	particles.SetData(randomParticles);
}

}
