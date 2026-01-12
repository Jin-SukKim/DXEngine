#pragma once
#include "ParticleModule.h"

namespace DE {
class ForceModule : public ParticleModule
{
public:
	void OnSpawn(SimulationContext& ctx) override;
	ModulePriority GetPriority() override { return ModulePriority::Force; }

	void LoadFromJson(const json& data) override;
public:
	float time = 0.f;
	Vector3 velocity = { 0.0f, 0.1f, 0.0f };; // 속도와 방향
	Vector2 speedRange = { 0.01f, 0.02f }; // 속도 범위
	Vector3 randomDir = { 0.2f, 0.5f, 0.2f }; // Random 추가 방향
	Vector3 gravity = { 0.f, 1.f, 0.f };
	float drag = 0.f;
};

}
