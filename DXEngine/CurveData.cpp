#include "pch.h"
#include "CurveData.h"

namespace DE {
	const float CurveData::PI = 3.14159265359f;

	CurveData::CurveData(LUTResolution res) : m_res(res), m_simplex(0)
	{
	}

	void CurveData::SetCurveType(CurveType type)
	{
		m_type = type;
		m_changed = true;
	}

	void CurveData::SetLURResolution(LUTResolution res)
	{
		m_res = res;
		m_changed = true;
	}

	void CurveData::SetCurveParam(CurveParams param)
	{
		if (m_param.seed != param.seed)
			m_simplex = SimplexNoise(param.seed);

		m_param = param;
		m_changed = true;
	}

	void CurveData::AddKeyFrame(KeyFrame keyFrame)
	{
		m_keyFrames.push_back(keyFrame);
		SortKeyFrames();
		m_changed = true;
	}

	void CurveData::UpdateKeyFrame(UINT slot, KeyFrame keyFrame)
	{
		if (slot < m_keyFrames.size()) {
			m_keyFrames[slot] = keyFrame;
			SortKeyFrames();
			m_changed = true;
		}
	}

	void CurveData::RemoveKeyFrame(UINT slot)
	{
		if (slot < m_keyFrames.size()) {
			m_keyFrames.erase(m_keyFrames.begin() + slot);
			m_changed = true;
		}
	}

	const std::vector<float>& CurveData::CreateCurveData()
	{
		if (!m_changed)
			return m_bakedData;

		UINT res = static_cast<UINT>(m_res);
		m_bakedData.resize(res);

		for (UINT i = 0; i < res; ++i) {
			// [0.0, 1.0]
			float t = (float)i / (res - 1);
			float val = Evaluate(t);
			m_bakedData[i] = std::clamp(val, m_minVal, m_maxVal);
		}

		m_changed = false;
		return m_bakedData;
	}

	CurveData CurveData::FromJson(const json& data, LUTResolution defaultRes)
	{
		CurveData curve(defaultRes);

		// Parse curve type
		if (data.contains("type")) {
			std::string typeStr = data["type"];
			if (typeStr == "LINEAR")      curve.SetCurveType(CurveType::LINEAR);
			else if (typeStr == "BEZIER") curve.SetCurveType(CurveType::BEZIER);
			else if (typeStr == "SIN")    curve.SetCurveType(CurveType::SIN);
			else if (typeStr == "COS")    curve.SetCurveType(CurveType::COS);
			else if (typeStr == "STEP")   curve.SetCurveType(CurveType::STEP);
			else if (typeStr == "NOISE")  curve.SetCurveType(CurveType::NOISE);
		}

		// Parse resolution
		if (data.contains("resolution")) {
			UINT res = data["resolution"];
			if (res <= 64)       curve.SetLURResolution(LUTResolution::Low);
			else if (res <= 128) curve.SetLURResolution(LUTResolution::Medium);
			else if (res <= 256) curve.SetLURResolution(LUTResolution::High);
			else                 curve.SetLURResolution(LUTResolution::Ultra);
		}

		// Parse curve params
		if (data.contains("params")) {
			auto& p = data["params"];
			CurveParams params;
			if (p.contains("frequency"))  params.frequency = p["frequency"];
			if (p.contains("amplitude"))  params.amplitude = p["amplitude"];
			if (p.contains("offset"))     params.offset = p["offset"];
			if (p.contains("seed"))       params.seed = p["seed"];
			curve.SetCurveParam(params);
		}

		// Parse keyframes
		if (data.contains("keyframes") && data["keyframes"].is_array()) {
			for (auto& kf : data["keyframes"]) {
				KeyFrame keyFrame;
				keyFrame.key = kf.value("key", 0.f);
				keyFrame.value = kf.value("value", 0.f);
				keyFrame.inTangent = kf.value("inTangent", 0.f);
				keyFrame.outTangent = kf.value("outTangent", 0.f);
				curve.AddKeyFrame(keyFrame);
			}
		}

		return curve;
	}

	float CurveData::Evaluate(float t) const
	{
		switch (m_type)
		{
		case CurveType::LINEAR: return EvaluateLinear(t);
		case CurveType::BEZIER: return EvaluateBezier(t);
		case CurveType::STEP:   return EvaluateStep(t);
		case CurveType::SIN:    return EvaluateSin(t);
		case CurveType::COS:    return EvaluateCos(t);
		case CurveType::NOISE:  return EvaluateNoise(t);
		default:                return 0.f;
		}
	}

	float CurveData::EvaluateLinear(float t) const
	{
		if (m_keyFrames.empty())
			return 0.f;

		if (m_keyFrames.size() == 1)
			return m_keyFrames[0].value;

		// t를 감싸는 두 키프레임 탐색
		if (t <= m_keyFrames.front().key) {
			// 첫 키프레임이 t=0에 있으면 그 값을 바로 반환
			if (m_keyFrames.front().key <= 0.f)
				return m_keyFrames.front().value;

			// 암묵적 (0, 0) -> 첫 키프레임까지 선형 보간
			float localT = t / m_keyFrames.front().key;
			return localT * m_keyFrames.front().value;
		}

		if (t >= m_keyFrames.back().key)
			return m_keyFrames.back().value;

		auto hi = std::upper_bound(m_keyFrames.begin(), m_keyFrames.end(), t,
			[](float val, const KeyFrame& kf)
			{ return val < kf.key; });

		auto lo = std::prev(hi);

		float dt = hi->key - lo->key;
		if (dt <= 0.0001f) return lo->value; // 겹쳤을 땐 이전 값 바로 반환

		float localT = (t - lo->key) / dt;
		return lo->value + localT * (hi->value - lo->value);
	}


	float CurveData::EvaluateBezier(float t) const
	{
		if (m_keyFrames.empty())     return 0.f;
		if (m_keyFrames.size() == 1) return m_keyFrames[0].value;

		if (t <= m_keyFrames.front().key)
		{
			if (m_keyFrames.front().key <= 0.f)
				return m_keyFrames.front().value;
			// 암묵적 (0, 0) → 첫 키프레임까지 Cubic Hermite 보간
			// 원점 outTangent = 0, 첫 키프레임 inTangent 사용
			float dt = m_keyFrames.front().key;
			if (dt <= 0.0001f) return m_keyFrames.front().value;
			float localT = t / dt;
			float t2 = localT * localT;
			float t3 = t2 * localT;

			float h00 = 2.f * t3 - 3.f * t2 + 1.f;
			float h10 = t3 - 2.f * t2 + localT;
			float h01 = -2.f * t3 + 3.f * t2;
			float h11 = t3 - t2;

			return h00 * 0.f
				+ h10 * dt * 0.f
				+ h01 * m_keyFrames.front().value
				+ h11 * dt * m_keyFrames.front().inTangent;
		}
		if (t >= m_keyFrames.back().key)
			return m_keyFrames.back().value;

		auto hi = std::upper_bound(m_keyFrames.begin(), m_keyFrames.end(), t,
			[](float val, const KeyFrame& kf)
			{ return val < kf.key; });
		auto lo = std::prev(hi);

		float dt = hi->key - lo->key;
		if (dt <= 0.0001f) return lo->value; // 겹쳤을 땐 이전 값 바로 반환

		float localT = (t - lo->key) / dt;
		float t2 = localT * localT;
		float t3 = t2 * localT;

		float h00 = 2.f * t3 - 3.f * t2 + 1.f;
		float h10 = t3 - 2.f * t2 + localT;
		float h01 = -2.f * t3 + 3.f * t2;
		float h11 = t3 - t2;

		return h00 * lo->value
			+ h10 * dt * lo->outTangent
			+ h01 * hi->value
			+ h11 * dt * hi->inTangent;
	}

	float CurveData::EvaluateStep(float t) const
	{
		if (m_keyFrames.empty()) return 0.f;

		// 첫 키프레임 이전 → 암묵적 원점(0) 유지
		if (t < m_keyFrames.front().key)
			return 0.f;
		if (t >= m_keyFrames.back().key)
			return m_keyFrames.back().value;

		float result = m_keyFrames.front().value;
		for (const auto& kf : m_keyFrames)
		{
			if (kf.key > t) break;
			result = kf.value;
		}
		return result;
	}

	float CurveData::EvaluateSin(float t) const
	{
		// 키프레임이 없으면 amplitude를 고정 Envelope으로 사용
		// 키프레임이 있으면 선형 보간값이 Envelope 역할
		float envelope = m_keyFrames.empty()
			? m_param.amplitude
			: EvaluateLinear(t) * m_param.amplitude;

		return envelope * std::sin(m_param.frequency * t * 2.f * PI + m_param.offset);
	}

	float CurveData::EvaluateCos(float t) const
	{
		float envelope = m_keyFrames.empty()
			? m_param.amplitude
			: EvaluateLinear(t) * m_param.amplitude;

		return envelope * std::cos(m_param.frequency * t * 2.f * PI + m_param.offset);
	}

	float CurveData::EvaluateNoise(float t) const
	{
		// 1D 커브이므로 Noise2D(t, offset)으로 샘플링
		// offset을 두 번째 축으로 사용 → seed처럼 다른 노이즈 패턴 생성
		float noiseVal = m_simplex.Noise2D(
			t * m_param.frequency,
			m_param.offset
		);
		
		// Noise2D 출력은 [-1, 1] → [0, 1]로 정규화
		noiseVal = noiseVal * 0.5f + 0.5f;

		float envelope = m_keyFrames.empty()
			? m_param.amplitude
			: EvaluateLinear(t) * m_param.amplitude;

		return envelope * noiseVal;
	}

	void CurveData::SortKeyFrames()
	{
		std::sort(m_keyFrames.begin(), m_keyFrames.end(), [](const KeyFrame& a, const KeyFrame& b) {
			return a.key < b.key;
			});
	}


}