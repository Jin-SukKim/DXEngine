#pragma once
#include "Actor.h"

namespace DE {
	class CameraActor : public Actor
	{
		using Super = Actor;
	public:
		CameraActor(const std::wstring& name) : Super(name) {}
		virtual ~CameraActor() {}

		Matrix GetViewMatrix();
		Matrix GetProjMatrix();

		void SetFovAngleY(const float& degree) { m_projFovAngleY = degree; }
		void SetNearZ(const float& z) { m_nearZ = z; }
		void SetFarZ(const float& z) { m_farZ = z; }
		void SetAspectRatio(float aspect) { m_aspect = aspect; };
		void UsePerspectiveProjection(const bool& use) { m_usePerspectiveProjection = use; }
	private:
		// Projection 옵션
		//float m_projFovAngleY = 90.f * 0.5f; // Luna 교재 기본 설정 (FOV)
		float m_projFovAngleY = 70.f;
		float m_nearZ = 0.01f;
		float m_farZ = 100.f;
		float m_aspect = 16.f / 9.f;
		bool m_usePerspectiveProjection = true; // 원근 투영
	};
}
