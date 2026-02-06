#pragma once
#include "ParticleModule.h"

namespace DE {

class VortexModule : public ParticleModule
{
public:
	void Initialize(ParticleInitContext& ctx) override;

	ModulePriority GetPriority() override { return ModulePriority::UpdateForce; }

	void LoadFromJson(const json& data) override;
	std::unique_ptr<ParticleModule> Clone() const override;
private:
	Vector3 m_vortexCenter = Vector3(0.f);
	float m_vortexStrength = 0.f;
	float m_vortexFalloff = 1.f;
	Vector3 m_vortexAxis = Vector3(0.f, 1.f, 0.f);
	Vector2 m_vortexPull = Vector2(0.f);

	// ComputeShader 제거 - ComputeCommon 사용
};

}
