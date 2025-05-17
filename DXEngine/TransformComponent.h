#pragma once
#include "Component.h"

namespace DE {
	class TransformComponent : public Component
	{
		using Super = Component;
	public:
		TransformComponent(const std::wstring& name) : Super(name) {}
		~TransformComponent() override {}

		void SetPos(const Vector3& pos) { m_pos = pos; }
		Vector3& GetPos() { return m_pos; }

		void SetScale(const Vector3& scale) { m_scale = scale; }
		Vector3& GetScale() { return m_scale; }

		void SetRotation(const float& yaw, const float& pitch, const float roll);
		void RotateRoll(const float& degree);
		void RotatePitch(const float& degree);
		void RotateYaw(const float& degree);
		
		Vector3 GetForwardDir();
		Vector3 GetRightDir();
		Vector3 GetUpDir();

		Matrix GetTransformMatrix(); // 변환 행렬
		Matrix GetInvTransformMatrix(); // 역변환 행렬

	private:
		Quaternion createRotationQuaternion(const float& yaw, const float& pitch, const float roll);

	private:
		Vector3 m_pos = Vector3::Zero;
		Vector3 m_scale = Vector3::One;
		Quaternion m_rotation;

		Vector3 m_forward = Vector3::UnitZ;
		Vector3 m_right = Vector3::UnitX;
		Vector3 m_up = Vector3::UnitY;
	};
}
