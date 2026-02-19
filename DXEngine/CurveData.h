#pragma once
#include "SimplexNoise.h"

namespace DE {
struct KeyFrame {
	float key;
	float value;

	// BEZIER CURVE ¿ë
	float inTangent = 0.f;
	float outTangent = 0.f;
};

struct CurveParams {
	float frequency = 1.f;
	float amplitude = 1.f;
	float offset = 0.f;
	UINT seed = 0; // Noise Seed
};

enum class CurveType {
	SIN,
	COS,
	LINEAR,
	BEZIER,
	STEP,
	NOISE
};

enum class ParticleCurveType {
	COLOR,
	ALPHA,
	SIZE,
	VELOCITY,
	DRAG,
	GRAVITY,
	NOISE_STRENTH,
	COUNT
};

class CurveData
{
public:
	enum class LUTResolution : UINT {
		Low = 64,
		Medium = 128,
		High = 256,
		Ultra = 512
	};

	CurveData(LUTResolution res);
	void SetCurveType(CurveType type);
	void SetLURResolution(LUTResolution res);
	void SetCurveParam(CurveParams param);

	void AddKeyFrame(KeyFrame keyFrame);
	void UpdateKeyFrame(UINT slot, KeyFrame keyFrame);
	void RemoveKeyFrame(UINT slot);

	const std::vector<float>& CreateCurveData();

private:
	float Evaluate(float t) const;
	float EvaluateLinear(float t) const;
	float EvaluateBezier(float t) const;
	float EvaluateStep(float t) const;
	float EvaluateSin(float t) const;
	float EvaluateCos(float t) const;
	float EvaluateNoise(float t) const;
	void  SortKeyFrames();

private:
	CurveType m_type = CurveType::LINEAR;
	LUTResolution m_res = LUTResolution::Medium;
	CurveParams m_param;
	std::vector<KeyFrame> m_keyFrames;
	std::vector<float> m_bakedData;
	SimplexNoise m_simplex;
	float m_minVal = 0.f;
	float m_maxVal = 1.f;
	bool m_changed = false;

	static const float PI;
};
}

