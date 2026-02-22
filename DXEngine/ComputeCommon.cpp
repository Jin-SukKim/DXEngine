#include "pch.h"
#include "ComputeCommon.h"

namespace DE {
	void ComputeCommon::InitCommonStates(ComPtr<ID3D11Device>& device)
	{
		initParticleShaders(device);
		initBitonicSortShaders(device);
		initComputePSOs(device);
	}

	void ComputeCommon::initParticleShaders(ComPtr<ID3D11Device>& device)
	{
		// Particle System Compute Shaders
		D3D11Utils::CreateCS(device.Get(), L"ParticleArgsUpdateCS.hlsl", particleArgsUpdateCS);
		D3D11Utils::CreateCS(device.Get(), L"ParticleMeshArgsUpdateCS.hlsl", particleMeshArgsUpdateCS);
		D3D11Utils::CreateCS(device.Get(), L"SpawnCS.hlsl", spawnCS);
		D3D11Utils::CreateCS(device.Get(), L"ParticleCS.hlsl", particleCS);
		//D3D11Utils::CreateCS(device.Get(), L"VortexCS.hlsl", vortexCS);
		//D3D11Utils::CreateCS(device.Get(), L"OrbitCS.hlsl", orbitCS);
		D3D11Utils::CreateCS(device.Get(), L"RenderArgsUpdateCS.hlsl", renderArgsUpdateCS);
		D3D11Utils::CreateCS(device.Get(), L"BatchRenderArgsCS.hlsl", batchRenderArgsCS);
		D3D11Utils::CreateCS(device.Get(), L"BuildAliveIndicesCS.hlsl", buildAliveIndicesCS);
		D3D11Utils::CreateCS(device.Get(), L"CopySortedIndicesCS.hlsl", copySortedIndicesCS);
		D3D11Utils::CreateCS(device.Get(), L"GenerateSortKeysCS.hlsl", generateSortKeysCS);

		// Depth shaders
		D3D11Utils::CreateCS(device.Get(), L"DepthDownsampleCS.hlsl", depthDownsampleCS);
	}

	void ComputeCommon::initBitonicSortShaders(ComPtr<ID3D11Device>& device)
	{
		// BitonicSort Compute Shaders
		D3D11Utils::CreateCS(device.Get(), L"BitonicSortCS.hlsl", bitonicSortCS);
		D3D11Utils::CreateCS(device.Get(), L"BitonicBlockSortCS.hlsl", bitonicBlockSortCS);
		D3D11Utils::CreateCS(device.Get(), L"BitonicInnerSortCS.hlsl", bitonicInnerSortCS);
	}

	void ComputeCommon::initComputePSOs(ComPtr<ID3D11Device>& device)
	{
		// Particle System PSOs
		particle.argsUpdateCS.computeShader = particleArgsUpdateCS;
		particle.meshArgsUpdateCS.computeShader = particleMeshArgsUpdateCS;
		particle.spawnCS.computeShader = spawnCS;
		particle.particleCS.computeShader = particleCS;
		particle.vortexCS.computeShader = vortexCS;
		particle.orbitCS.computeShader = orbitCS;
		particle.renderArgsUpdateCS.computeShader = renderArgsUpdateCS;
		particle.batchRenderArgsCS.computeShader = batchRenderArgsCS;
		particle.buildAliveIndicesCS.computeShader = buildAliveIndicesCS;
		particle.copySortedIndicesCS.computeShader = copySortedIndicesCS;
		particle.generateSortKeysCS.computeShader = generateSortKeysCS;

		// BitonicSort PSOs
		sort.bitonicSortCS.computeShader = bitonicSortCS;
		sort.bitonicBlockSortCS.computeShader = bitonicBlockSortCS;
		sort.bitonicInnerSortCS.computeShader = bitonicInnerSortCS;

		// Depth PSO
		depth.depthDownsampleCS.computeShader = depthDownsampleCS;
	}
}