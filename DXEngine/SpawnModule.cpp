#include "pch.h"
#include "SpawnModule.h"
#include "ParticleEmitter.h"

namespace DE {
	void SpawnModule::Initialize(ID3D11Device* device, ParticleEmitter* owner)
	{
		ParticleModule::Initialize(device, owner);
		m_owner->GetConstsData().maxParticles = maxParticles;
		m_spawnCS.Initialize(device, L"SpawnCS.hlsl");
	}
	void SpawnModule::OnSpawn(ID3D11DeviceContext* context)
	{
		ParticleConsts& consts = m_owner->GetConstsData();
		consts.spawnVolume = spawnVolume;
		consts.lifeRange = lifeRange;
		consts.maxParticles = maxParticles;
	}

	void SpawnModule::PreUpdate(ID3D11DeviceContext* context, float dt)
	{
		spawnAccumulator += spawnRate * dt;

		UINT spawnCycles = static_cast<int>(spawnAccumulator);
		m_totalSpawnCount = spawnCycles * particlesPerSpawn;

		if (spawnCycles > 0)
			spawnAccumulator -= static_cast<float>(spawnCycles);
		if (m_totalSpawnCount < 0)
			m_totalSpawnCount = 0;

		m_owner->GetConstsData().spawnCount = m_totalSpawnCount;
	}
	
	void SpawnModule::OnUpdate(ID3D11DeviceContext* context, float dt)
	{
		if (m_totalSpawnCount == 0)
			return;

		m_spawnCS.UpdateConsts(context, 0, 1, m_owner->GetConstBuffer().GetAddressOf());

		ID3D11UnorderedAccessView* uav = m_owner->GetConsumeBuffer().GetUAV();
		context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);

		// Spawn Compute Shader
		UINT groupCount = (m_totalSpawnCount + 255) / 256;
		m_spawnCS.Dispatch(context, groupCount, 1, 1);
	}
}