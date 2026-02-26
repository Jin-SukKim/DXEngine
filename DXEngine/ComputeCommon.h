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
			ComputePSO meshArgsUpdateCS;      // ParticleMeshArgsUpdateCS.hlsl
			ComputePSO spawnCS;               // SpawnCS.hlsl
			ComputePSO particleCS;            // ParticleCS.hlsl (Force)
			ComputePSO vortexCS;              // VortexCS.hlsl
			ComputePSO orbitCS;              // OrbitCS.hlsl
			ComputePSO renderArgsUpdateCS;              // OrbitCS.hlsl
			ComputePSO batchRenderArgsCS;     // BatchRenderArgsCS.hlsl
			ComputePSO buildAliveIndicesCS;   // BuildAliveIndicesCS.hlsl
			ComputePSO copySortedIndicesCS;   // CopySortedIndicesCS.hlsl
			ComputePSO generateSortKeysCS;   // GenerateSortKeysCS.hlsl
			ComputePSO prepareSortDispatchCS; // PrepareSortDispatchCS.hlsl
		} particle;

		// BitonicSort
		struct {
			ComputePSO bitonicSortCS;         // BitonicSortCS.hlsl
			ComputePSO bitonicBlockSortCS;    // BitonicBlockSortCS.hlsl (LDS block sort)
			ComputePSO bitonicInnerSortCS;    // BitonicInnerSortCS.hlsl (LDS inner merge)
		} sort;

		// Depth operations
		struct {
			ComputePSO depthDownsampleCS;     // DepthDownsampleCS.hlsl
		} depth;

	private:
		// Compute Shaders
		ComPtr<ID3D11ComputeShader> particleArgsUpdateCS;
		ComPtr<ID3D11ComputeShader> particleMeshArgsUpdateCS;
		ComPtr<ID3D11ComputeShader> bitonicSortCS;
		ComPtr<ID3D11ComputeShader> bitonicBlockSortCS;
		ComPtr<ID3D11ComputeShader> bitonicInnerSortCS;
		ComPtr<ID3D11ComputeShader> spawnCS;
		ComPtr<ID3D11ComputeShader> particleCS;
		ComPtr<ID3D11ComputeShader> vortexCS;
		ComPtr<ID3D11ComputeShader> orbitCS;
		ComPtr<ID3D11ComputeShader> renderArgsUpdateCS;
		ComPtr<ID3D11ComputeShader> batchRenderArgsCS;
		ComPtr<ID3D11ComputeShader> buildAliveIndicesCS;
		ComPtr<ID3D11ComputeShader> copySortedIndicesCS;
		ComPtr<ID3D11ComputeShader> generateSortKeysCS;
		ComPtr<ID3D11ComputeShader> prepareSortDispatchCS;

		// Depth shaders
		ComPtr<ID3D11ComputeShader> depthDownsampleCS;
	};
}