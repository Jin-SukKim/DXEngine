#pragma once
#include "ParticleModule.h"

namespace DE {

class ForceModule : public ParticleModule
{
public:
	void Initialize(ParticleInitContext& ctx) override;
	void OnUpdate(const SimulationContext& context) override;

	ModulePriority GetPriority() override { return ModulePriority::Force; }

	void LoadFromJson(const json& data) override;
	std::unique_ptr<ParticleModule> Clone() const override;
public:
	Vector3 velocity = { 0.0f, 0.1f, 0.0f };
	Vector2 speedRange = { 0.01f, 0.02f };
	Vector3 randomDir = { 0.2f, 0.5f, 0.2f };
	Vector3 gravity = { 0.f, 1.f, 0.f };
	float drag = 0.f;

	float curlNoiseFrequency = 0.1f;
	float curlNoiseStrength = 1.0f;
	bool curlNoiseEnabled = false;

	// ComputeShader ���� - ComputeCommon ���
};

}
