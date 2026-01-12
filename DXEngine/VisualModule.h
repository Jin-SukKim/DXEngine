#pragma once
#include "ParticleModule.h"

namespace DE {
class VisualModule : public ParticleModule
{
public:
	void OnSpawn(ID3D11DeviceContext* context) override;
	void LoadFromJson(const json& data) override;
	ModulePriority GetPriority() override { return ModulePriority::Visual; }
public:
	Vector4 startColor = { 1.0f, 0.1f, 0.0f, 1.f };
	Vector4 endColor = { 1.0f, 0.8f, 0.1f, 1.f };
	Vector2 sizeRange = { 0.25f, 0.05f };
};
}

