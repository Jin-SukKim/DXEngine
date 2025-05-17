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


		void SetAspectRatio(float aspect) { m_aspect = aspect; };

	private:
		// Projection �ɼ�
		float m_projFovAngleY = 90.f * 0.5f; // Luna ���� �⺻ ���� (FOV)
		float m_nearZ = 0.01f;
		float m_farZ = 100.f;
		float m_aspect = 16.f / 9.f;
		bool m_usePerspectiveProjection = true; // ���� ����
	};
}
