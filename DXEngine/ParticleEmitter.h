#pragma once
#include "Actor.h"
#include "Particle.h"
#include "ComputeShader.h"
#include "AppendBuffer.h"
#include "StagingBuffer.h"
#include "IndirectArgsBuffer.h"

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
		AppendBuffer<Particle> m_append;
		AppendBuffer<Particle> m_consume;

		IndirectArgsBuffer<DispatchArgs> m_dispatchArgs;
		IndirectArgsBuffer<DrawInstancedArgs> m_drawInstancedArgs;
		ComPtr<ID3D11Buffer> m_countBuffer;
		ComPtr<ID3D11ShaderResourceView> m_countSRV;

		ComputeShader m_spawnCS;
		ComputeShader m_argsUpdateCS;
		ComputeShader m_particleCS;
		ConstantBuffer<ParticleConsts> m_consts;

		// 최소한의 상태만 관리
		UINT maxParticles = 1024;
		float m_elapsedTime = 0.0f;
		float m_targetSpawnRate = 5.0f;
		float m_spawnAccumulator = 0.f;
	};
}