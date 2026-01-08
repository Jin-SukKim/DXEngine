#pragma once
#include "Actor.h"
#include "StructuredBuffer.h"
#include "Particle.h"
#include "ComputeShader.h"

namespace DE {
	class ParticleEmitter : public Actor
	{
	public:
		ParticleEmitter(const std::wstring& name);
		~ParticleEmitter() override {}
		void Initialize() override;
		void Update(const float& dt) override;
		void Render() override;

	private:
		void GenerateRandomParticles(StructuredBuffer<Particle>& particles);

	private:
		StructuredBuffer<Particle> m_particles;
		ComputeShader m_particleCS;
		
		ConstantBuffer<ParticleConsts> m_consts;
	};
}

