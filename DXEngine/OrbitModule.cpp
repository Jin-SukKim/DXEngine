#include "pch.h"
#include "OrbitModule.h"
#include "ParticleEmitter.h"

namespace DE {
	void OrbitModule::Initialize(ParticleInitContext& ctx)
	{
		// ConstantBuffer ����ü�� Orbit ���� �ʵ尡 �߰��Ǿ�� �մϴ�.
		auto& consts = ctx.consts.orbit;
		consts.center = m_center;
		consts.axis = m_axis;
		consts.rotationRate = m_rotationRate; // ���̴����� dt�� ���� ���
		consts.initialOffset = m_initialOffset;
		consts.active = 1;
	}

	void OrbitModule::LoadFromJson(const json& data)
	{
		if (data.contains("center")) m_center = JsonToVec3(data["center"]);
		if (data.contains("axis")) m_axis = JsonToVec3(data["axis"]);
		if (data.contains("rotationRate")) m_rotationRate = data["rotationRate"];
		if (data.contains("offset")) m_initialOffset = data["offset"];
		if (data.contains("rateCurve")) {
			m_rateCurve = CurveData::FromJson(data["rateCurve"]);
			m_hasRateCurve = true;
		}
	}

	void OrbitModule::CollectCurves(std::unordered_map<ParticleCurveType, CurveData>& curves)
	{
		if (m_hasRateCurve) curves.emplace(ParticleCurveType::ORBIT_RATE, m_rateCurve);
	}

	std::unique_ptr<ParticleModule> OrbitModule::Clone() const
	{
		auto cloned = std::make_unique<OrbitModule>();
		cloned->m_center = this->m_center;
		cloned->m_axis = this->m_axis;
		cloned->m_rotationRate = this->m_rotationRate;
		cloned->m_initialOffset = this->m_initialOffset;
		cloned->m_isEnabled = this->m_isEnabled;
		cloned->m_rateCurve = this->m_rateCurve;
		cloned->m_hasRateCurve = this->m_hasRateCurve;
		return cloned;
	}
}