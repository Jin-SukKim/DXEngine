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
		~ParticleEmitter() override = default;

		void Initialize() override;
		void Update(const float& dt) override;
		void Render() override;

		// 설정 메서드
		void SetMaxParticles(UINT count) { maxParticles = count; }
		void SetSpawnRate(float rate) { m_targetSpawnRate = rate; }
		
		UINT GetMaxParticles() const { return maxParticles; }
		float GetSpawnRate() const { return m_targetSpawnRate; }

		void SetBurst(UINT count);
		void SetParticlesPerSpawn(UINT count);
		void SetParticleConfig(const ParticleConsts& config);
		void SetupFire();
		void SetupExplosion();
	private:
		// 초기화 헬퍼 함수들
		void InitializeShaders(ID3D11Device* device);
		void InitializeBuffers(ID3D11Device* device);

		// 업데이트 단계별 함수들
		void UpdateSpawnStage(ID3D11DeviceContext* context, float dt);
		void UpdateArgsBuffers(ID3D11DeviceContext* context);
		void UpdateSimulationStage(ID3D11DeviceContext* context);

		// 파티클 버퍼 (핑퐁 버퍼링)
		AppendBuffer<Particle> m_consume;
		AppendBuffer<Particle> m_append;

		// 간접 디스패치 및 드로우 인수
		IndirectArgsBuffer<DispatchArgs> m_dispatchArgs;
		IndirectArgsBuffer<DrawInstancedArgs> m_drawInstancedArgs;
		
		// 활성 파티클 개수 추적용 카운터 버퍼
		ComPtr<ID3D11Buffer> m_countBuffer;
		ComPtr<ID3D11ShaderResourceView> m_countSRV;

		// 컴퓨트 셰이더들
		ComputeShader m_spawnCS;
		ComputeShader m_argsUpdateCS;
		ComputeShader m_particleCS;
		
		// 상수 버퍼
		ConstantBuffer<ParticleConsts> m_consts;

		// 파티클 시스템 파라미터
		UINT maxParticles = 1024;
		float m_targetSpawnRate = 1.0f;
		UINT m_burstCount = 0;
		
		// 런타임 상태
		float m_elapsedTime = 0.0f;
		float m_spawnAccumulator = 0.0f;
		UINT m_particlePerSpawn = 1;
	};
}