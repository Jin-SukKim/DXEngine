#include "pch.h"
#include "ForceModule.h"
#include "ParticleEmitter.h"

namespace DE {
	void ForceModule::Initialize(ParticleInitContext& ctx)
	{
		// ��� �� �ʱ�ȭ (OnSpawn���� �̵�)
		ForceConsts& consts = ctx.consts.force;
		consts.velocity = velocity;
		consts.speedRange = speedRange;
		consts.randomDir = randomDir;
		consts.gravity = gravity;
		consts.drag = drag;
		consts.curlNoiseFrequency = curlNoiseFrequency;
		consts.curlNoiseStrength = curlNoiseStrength;
		consts.curlNoiseEnabled = curlNoiseEnabled ? 1 : 0;
		consts.curlNoiseScrollSpeed = curlNoiseScrollSpeed;
	}

	void ForceModule::OnUpdate(const SimulationContext& ctx)
	{
		ParticleModule::OnUpdate(ctx);

	}

	void ForceModule::LoadFromJson(const json& data)
	{
		if (data.contains("velocity")) velocity = JsonToVec3(data["velocity"]);
		if (data.contains("speedRange")) speedRange = JsonToVec2(data["speedRange"]);
		if (data.contains("randomDir")) randomDir = JsonToVec3(data["randomDir"]);
		if (data.contains("gravity")) gravity = JsonToVec3(data["gravity"]);
		if (data.contains("drag")) drag = data["drag"];
		if (data.contains("curlNoiseFrequency")) curlNoiseFrequency = data["curlNoiseFrequency"];
		if (data.contains("curlNoiseStrength")) curlNoiseStrength = data["curlNoiseStrength"];
		if (data.contains("curlNoiseEnabled")) curlNoiseEnabled = data["curlNoiseEnabled"];
		if (data.contains("curlNoiseScrollSpeed")) curlNoiseScrollSpeed = JsonToVec3(data["curlNoiseScrollSpeed"]);

		if (data.contains("velocityCurve")) {
			m_velocityCurve = CurveData::FromJson(data["velocityCurve"]);
			m_hasVelocityCurve = true;
		}
		if (data.contains("dragCurve")) {
			m_dragCurve = CurveData::FromJson(data["dragCurve"]);
			m_hasDragCurve = true;
		}
		if (data.contains("gravityCurve")) {
			m_gravityCurve = CurveData::FromJson(data["gravityCurve"]);
			m_hasGravityCurve = true;
		}
		if (data.contains("noiseStrengthCurve")) {
			m_noiseStrengthCurve = CurveData::FromJson(data["noiseStrengthCurve"]);
			m_hasNoiseStrengthCurve = true;
		}
	}

	void ForceModule::CollectCurves(std::unordered_map<ParticleCurveType, CurveData>& curves)
	{
		if (m_hasVelocityCurve)      curves.emplace(ParticleCurveType::VELOCITY, m_velocityCurve);
		if (m_hasDragCurve)          curves.emplace(ParticleCurveType::DRAG, m_dragCurve);
		if (m_hasGravityCurve)       curves.emplace(ParticleCurveType::GRAVITY, m_gravityCurve);
		if (m_hasNoiseStrengthCurve) curves.emplace(ParticleCurveType::NOISE_STRENTH, m_noiseStrengthCurve);
	}

	std::unique_ptr<ParticleModule> ForceModule::Clone() const
	{
		auto cloned = std::make_unique<ForceModule>();

		cloned->velocity = this->velocity;
		cloned->speedRange = this->speedRange;
		cloned->randomDir = this->randomDir;
		cloned->drag = this->drag;
		cloned->gravity = this->gravity;
		cloned->curlNoiseFrequency = this->curlNoiseFrequency;
		cloned->curlNoiseStrength = this->curlNoiseStrength;
		cloned->curlNoiseEnabled = this->curlNoiseEnabled;
		cloned->m_isEnabled = this->m_isEnabled;

		cloned->m_velocityCurve = this->m_velocityCurve;
		cloned->m_dragCurve = this->m_dragCurve;
		cloned->m_gravityCurve = this->m_gravityCurve;
		cloned->m_noiseStrengthCurve = this->m_noiseStrengthCurve;
		cloned->m_hasVelocityCurve = this->m_hasVelocityCurve;
		cloned->m_hasDragCurve = this->m_hasDragCurve;
		cloned->m_hasGravityCurve = this->m_hasGravityCurve;
		cloned->m_hasNoiseStrengthCurve = this->m_hasNoiseStrengthCurve;

		return cloned;
	}
}