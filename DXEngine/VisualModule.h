#pragma once
#include "ParticleModule.h"

namespace DE {
class VisualModule : public ParticleModule
{
public:
	void Initialize(ParticleInitContext& ctx) override;
	void LoadFromJson(const json& data) override;
	ModulePriority GetPriority() override { return ModulePriority::Visual; }
	std::unique_ptr<ParticleModule> Clone() const override;
private:
	Vector4 startColor = { 1.0f, 0.1f, 0.0f, 1.f };
	Vector4 endColor = { 1.0f, 0.8f, 0.1f, 1.f };
	Vector2 sizeRange = { 0.25f, 0.05f };
	
	Vector3 minRotation = Vector3(0.f);
	Vector3 maxRotation = Vector3(360.f);
	Vector3 minRotSpeed = Vector3(-1.f);
	Vector3 maxRotSpeed = Vector3(1.f);
};
}

