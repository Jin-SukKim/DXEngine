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
	void CollectCurves(std::unordered_map<ParticleCurveType, CurveData>& curves) override;
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
	Vector3 curlNoiseScrollSpeed = Vector3(0.f, 0.f, 0.f);
	
	// CurveData
	CurveData m_velocityCurve{ CurveData::LUTResolution::Medium };
	CurveData m_dragCurve{ CurveData::LUTResolution::Medium };
	CurveData m_gravityCurve{ CurveData::LUTResolution::Medium };
	CurveData m_noiseStrengthCurve{ CurveData::LUTResolution::Medium };
	bool m_hasVelocityCurve = false;
	bool m_hasDragCurve = false;
	bool m_hasGravityCurve = false;
	bool m_hasNoiseStrengthCurve = false;
};

}
