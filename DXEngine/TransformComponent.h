#pragma once
#include "Component.h"

namespace DE {
	class TransformComponent : public Component
	{
		using Super = Component;
	public:
		TransformComponent(const std::wstring& name) : Super(name, ComponentType::Transform) {}
		~TransformComponent() override {}

		void SetPos(const Vector3& pos) { m_pos = pos; }
		Vector3& GetPos() { return m_pos; }
		void Translate(const Vector3& pos) { m_pos += pos; }

		void SetScale(const Vector3& scale) { m_scale = scale; SetBoundingVolumeScale(); }
		Vector3& GetScale() { return m_scale; }

		void SetLocalRotation(const float& yaw, const float& pitch, const float roll);
		void SetLocalRotation(const Quaternion& q);
		void SetRotation(const float& yaw, const float& pitch, const float roll);
		void SetRotation(const Quaternion& q);
		void Rotate(const float& yaw, const float& pitch, const float roll);
		
		Vector3 GetForwardDir();
		Vector3 GetRightDir();
		Vector3 GetUpDir();

		Matrix GetTranslateMatrix();
		Matrix GetRotationMatrix();
		Matrix GetTransformMatrix(); // 변환 행렬
		Matrix GetInvTransformMatrix(); // 역변환 행렬

	private:
		Quaternion createRotationQuaternion(const float& yaw, const float& pitch, const float roll);
		void SetBoundingVolumeScale();
	private:
		Vector3 m_pos = Vector3(0.f);
		Vector3 m_scale = Vector3(1.f);
		Vector3 m_localRotation = Vector3(0.f);
		Quaternion m_localQuaternion;
		Vector3 m_worldRotation = Vector3(0.f); // yaw, pitch, roll
		Quaternion m_worldQuaternion;

		Vector3 m_localForward = Vector3(0.f, 0.f, 1.f);
		Vector3 m_localRight = Vector3(1.f, 0.f, 0.f);
		Vector3 m_localUp = Vector3(0.f, 1.f, 0.f);
	};
}
