#pragma once
#include "Actor.h"
#include "Particle.h"
#include "ComputeShader.h"
#include "AppendBuffer.h"
#include "StagingBuffer.h"

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
		AppendBuffer<Particle> m_append;
		AppendBuffer<Particle> m_consume;
		StagingBuffer<UINT> m_activeCount;

		ComputeShader m_particleCS;
		ConstantBuffer<ParticleConsts> m_consts;
	};
}

