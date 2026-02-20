#pragma once
#include "ParticleModule.h"

namespace DE {

	class OrbitModule : public ParticleModule
	{
	public:
		void Initialize(ParticleInitContext& ctx) override;

		// 위치를 강제로 수정하므로 업데이트 우선순위를 높게 잡거나, 물리 연산 이후(Post-Update)에 보정할 수도 있습니다.
		// 여기서는 일반 UpdateForce 단계에서 처리하되, 셰이더에서 직접 위치를 변환합니다.
		ModulePriority GetPriority() override { return ModulePriority::UpdateForce; }

		void LoadFromJson(const json& data) override;
		void CollectCurves(std::unordered_map<ParticleCurveType, CurveData>& curves) override;
		std::unique_ptr<ParticleModule> Clone() const override;

	public:
		Vector3 m_center = Vector3(0.f);          // 회전 중심
		Vector3 m_axis = Vector3(0.f, 1.f, 0.f);  // 회전 축 (예: Y축)
		float m_rotationRate = 1.0f;              // 회전 속도 (Rad/s or Deg/s)
		float m_initialOffset = 0.f;              // (옵션) 중심으로부터의 강제 거리

		CurveData m_rateCurve{ CurveData::LUTResolution::Medium };
		bool m_hasRateCurve = false;
	};

}