#pragma once
#include "ComputePSO.h"

namespace DE {
	class ComputeCommon {
	public:
		void InitCommonStates(ComPtr<ID3D11Device>& device);

	private:
		void initParticleShaders(ComPtr<ID3D11Device>& device);
		void initBitonicSortShaders(ComPtr<ID3D11Device>& device);
		void initComputePSOs(ComPtr<ID3D11Device>& device);

	public:
		// Particle System
		struct {
			ComputePSO argsUpdateCS;          // ParticleArgsUpdateCS.hlsl
			ComputePSO initSortKeysCS;        // InitBitonicSortCS.hlsl
			ComputePSO meshArgsUpdateCS;      // ParticleMeshArgsUpdateCS.hlsl
			ComputePSO spawnCS;               // SpawnCS.hlsl
			ComputePSO particleCS;            // ParticleCS.hlsl (Force)
			ComputePSO vortexCS;              // VortexCS.hlsl
			ComputePSO orbitCS;              // OrbitCS.hlsl
			ComputePSO renderArgsUpdateCS;              // RenderArgsUpdateCS.hlsl
		ComputePSO frustumCullingCS;                // ParticleFrustumCullingCS.hlsl
		} particle;

		// BitonicSort
		struct {
			ComputePSO bitonicSortCS;         // BitonicSortCS.hlsl
		} sort;

	private:
		// Compute Shaders
		ComPtr<ID3D11ComputeShader> particleArgsUpdateCS;
		ComPtr<ID3D11ComputeShader> particleMeshArgsUpdateCS;
		ComPtr<ID3D11ComputeShader> initBitonicSortCS;
		ComPtr<ID3D11ComputeShader> bitonicSortCS;
		ComPtr<ID3D11ComputeShader> spawnCS;
		ComPtr<ID3D11ComputeShader> particleCS;
		ComPtr<ID3D11ComputeShader> vortexCS;
		ComPtr<ID3D11ComputeShader> orbitCS;
		ComPtr<ID3D11ComputeShader> renderArgsUpdateCS;
	ComPtr<ID3D11ComputeShader> frustumCullingCS;
	};
}