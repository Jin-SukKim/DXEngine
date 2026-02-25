#pragma once
#include "Actor.h"
#include "InputAction.h"

namespace DE {
	class CameraActor : public Actor
	{
		using Super = Actor;
	public:
		CameraActor(const std::wstring& name) : Super(name) {}
		virtual ~CameraActor() {}

		void Initialize() override;
		void Update(const float& deltaTime) override;

		Matrix GetViewMatrix();
		Matrix GetProjMatrix();
		Vector3 GetPos();

		void SetFovAngleY(const float& degree) { m_projFovAngleY = degree; }
		void SetNearZ(const float& z) { m_nearZ = z; }
		void SetFarZ(const float& z) { m_farZ = z; }
		void SetAspectRatio(float aspect) { m_aspect = aspect; };
		void UsePerspectiveProjection(const bool& use) { m_usePerspectiveProjection = use; }
		void SetFPV(const bool& fpv) { m_fpv = fpv; }
		void EnableFPV() { m_fpv = !m_fpv; }

	private:
		// Projection 옵션
		//float m_projFovAngleY = 90.f * 0.5f; // Luna 교재 기본 설정 (FOV)
		float m_projFovAngleY = 70.0f; // Luna 교재 기본 설정 (FOV)
		float m_nearZ = 0.01f;
		float m_farZ = 1000.f;
		float m_aspect = 16.f / 9.f;
		bool m_usePerspectiveProjection = true; // 원근 투영

		bool m_fpv = false;
		float m_speed = 10.f;
		float m_rotateSpeed = 35.f;

		Vector2 m_prevMousePos = Vector2(0.f, 0.f);
		InputAxis zAxis = InputAxis::ZAxis;
		InputAxis xAxis = InputAxis::XAxis;
		InputAxis yAxis = InputAxis::YAxis;
		InputButton w = InputButton::W, s = InputButton::S;
		InputButton a = InputButton::A, d = InputButton::D;
		InputButton q = InputButton::Q, e = InputButton::E;
		InputAxisAction m_forward;
		InputAxisAction m_right;
		InputAxisAction m_up;
		InputButton lButton = InputButton::LButton;
		InputButton rButton = InputButton::RButton;
		InputAxisAction m_mouseClick;
	};
}
