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
		D3D11Utils::CreateCS(device.Get(), L"InitBitonicSortCS.hlsl", initBitonicSortCS);
		D3D11Utils::CreateCS(device.Get(), L"SpawnCS.hlsl", spawnCS);
		D3D11Utils::CreateCS(device.Get(), L"ParticleCS.hlsl", particleCS);
		//D3D11Utils::CreateCS(device.Get(), L"VortexCS.hlsl", vortexCS);
		//D3D11Utils::CreateCS(device.Get(), L"OrbitCS.hlsl", orbitCS);
		D3D11Utils::CreateCS(device.Get(), L"RenderArgsUpdateCS.hlsl", renderArgsUpdateCS);
		D3D11Utils::CreateCS(device.Get(), L"BatchArgsUpdateCS.hlsl", batchArgsUpdateCS);
	}

	void ComputeCommon::initBitonicSortShaders(ComPtr<ID3D11Device>& device)
	{
		// BitonicSort Compute Shader
		D3D11Utils::CreateCS(device.Get(), L"BitonicSortCS.hlsl", bitonicSortCS);
	}

	void ComputeCommon::initComputePSOs(ComPtr<ID3D11Device>& device)
	{
		// Particle System PSOs
		particle.argsUpdateCS.computeShader = particleArgsUpdateCS;
		particle.initSortKeysCS.computeShader = initBitonicSortCS;
		particle.meshArgsUpdateCS.computeShader = particleMeshArgsUpdateCS;
		particle.spawnCS.computeShader = spawnCS;
		particle.particleCS.computeShader = particleCS;
		particle.vortexCS.computeShader = vortexCS;
		particle.orbitCS.computeShader = orbitCS;
		particle.renderArgsUpdateCS.computeShader = renderArgsUpdateCS;
		particle.batchArgsUpdateCS.computeShader = batchArgsUpdateCS;

		// BitonicSort PSO
		sort.bitonicSortCS.computeShader = bitonicSortCS;
	}
}