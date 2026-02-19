#pragma once
#include "ParticleModule.h"

namespace DE {

	class OrbitModule : public ParticleModule
	{
	public:
		void Initialize(ParticleInitContext& ctx) override;
		void CollectCurves(std::unordered_map<ParticleCurveType, CurveData>& curves) override;

		// ��ġ�� ������ �����ϹǷ� ������Ʈ �켱������ ���� ��ų�, ���� ���� ����(Post-Update)�� ������ ���� �ֽ��ϴ�.
		// ���⼭�� �Ϲ� UpdateForce �ܰ迡�� ó���ϵ�, ���̴����� ���� ��ġ�� ��ȯ�մϴ�.
		ModulePriority GetPriority() override { return ModulePriority::UpdateForce; }

		void LoadFromJson(const json& data) override;
		std::unique_ptr<ParticleModule> Clone() const override;

	public:
		Vector3 m_center = Vector3(0.f);          // ȸ�� �߽�
		Vector3 m_axis = Vector3(0.f, 1.f, 0.f);  // ȸ�� �� (��: Y��)
		float m_rotationRate = 1.0f;              // ȸ�� �ӵ� (Rad/s or Deg/s)
		float m_initialOffset = 0.f;              // (�ɼ�) �߽����κ����� ���� �Ÿ�

		CurveData m_rateCurve{CurveData::LUTResolution::Medium};
		bool m_hasRateCurve = false;
	};

}