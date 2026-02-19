#include "pch.h"
#include "VisualModule.h"
#include "ParticleEmitter.h"

namespace DE {
	void VisualModule::Initialize(ParticleInitContext& ctx)
	{
		// ��� �� �ʱ�ȭ (OnSpawn���� �̵�)
		VisualConsts& consts = ctx.consts.visual;
		consts.startColor = startColor;
		consts.endColor = endColor;
		consts.sizeRange = sizeRange;
		consts.minRotation = minRotation;
		consts.maxRotation = maxRotation;
		consts.minRotSpeed = minRotSpeed;
		consts.maxRotSpeed = maxRotSpeed;
	}

	void VisualModule::LoadFromJson(const json& data)
	{
		if (data.contains("startColor")) startColor = JsonToVec4(data["startColor"]);
		if (data.contains("endColor")) endColor = JsonToVec4(data["endColor"]);
		if (data.contains("sizeRange")) sizeRange = JsonToVec2(data["sizeRange"]);
		if (data.contains("rotation")) {
			auto& rot = data["rotation"];
			if (rot.contains("minRotation")) minRotation = JsonToVec3(rot["minRotation"]);
			if (rot.contains("maxRotation")) maxRotation = JsonToVec3(rot["maxRotation"]);
			if (rot.contains("minRotSpeed")) minRotSpeed = JsonToVec3(rot["minRotSpeed"]);
			if (rot.contains("maxRotSpeed")) maxRotSpeed = JsonToVec3(rot["maxRotSpeed"]);
		}

		if (data.contains("colorCurve")) {
			m_colorCurve = CurveData::FromJson(data["colorCurve"]);
			m_hasColorCurve = true;
		}
		if (data.contains("alphaCurve")) {
			m_alphaCurve = CurveData::FromJson(data["alphaCurve"]);
			m_hasAlphaCurve = true;
		}
		if (data.contains("sizeCurve")) {
			m_sizeCurve = CurveData::FromJson(data["sizeCurve"]);
			m_hasSizeCurve = true;
		}
	}

	void VisualModule::CollectCurves(std::unordered_map<ParticleCurveType, CurveData>& curves)
	{
		if (m_hasColorCurve) curves.emplace(ParticleCurveType::COLOR, m_colorCurve);
		if (m_hasAlphaCurve) curves.emplace(ParticleCurveType::ALPHA, m_alphaCurve);
		if (m_hasSizeCurve)  curves.emplace(ParticleCurveType::SIZE, m_sizeCurve);
	}
	std::unique_ptr<ParticleModule> VisualModule::Clone() const
	{
		auto cloned = std::make_unique<VisualModule>();

		cloned->startColor = this->startColor;
		cloned->endColor = this->endColor;
		cloned->sizeRange = this->sizeRange;
		cloned->minRotation = this->minRotation;
		cloned->maxRotation = this->maxRotation;
		cloned->minRotSpeed = this->minRotSpeed;
		cloned->maxRotSpeed = this->maxRotSpeed;
		cloned->m_isEnabled = this->m_isEnabled;

		cloned->m_colorCurve = this->m_colorCurve;
		cloned->m_alphaCurve = this->m_alphaCurve;
		cloned->m_sizeCurve = this->m_sizeCurve;
		cloned->m_hasColorCurve = this->m_hasColorCurve;
		cloned->m_hasAlphaCurve = this->m_hasAlphaCurve;
		cloned->m_hasSizeCurve = this->m_hasSizeCurve;

		return std::move(cloned);
	}
}