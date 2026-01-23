#pragma once
#include "ParticleModule.h"
#include "ComputeShader.h"

namespace DE {

class VortexModule : public ParticleModule
{
public:
	void Initialize(ParticleInitContext& ctx) override;
	void OnSpawn(SimulationContext& ctx) override;
	void OnUpdate(const SimulationContext& context) override;

	ModulePriority GetPriority() override { return ModulePriority::UpdateForce; }

	void LoadFromJson(const json& data) override;
	std::unique_ptr<ParticleModule> Clone() const override;
public:
	Vector3 m_vortexCenter = Vector3(0.f);
	float m_vortexStrength = 0.f;
	float m_vortexFalloff = 1.f;
	Vector3 m_vortexAxis = Vector3(0.f, 1.f, 0.f);
	Vector2 m_vortexPull = Vector2(0.f);

	ComputeShader m_vortexCS;
};

}
