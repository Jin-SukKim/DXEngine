#include "pch.h"
#include "TransformComponent.h"
#include "Actor.h"
#include "BoundComponent.h"

namespace DE {
	using namespace DirectX;

	void TransformComponent::SetRotation(const float& yaw, const float& pitch, const float roll)
	{
		m_rotation = createRotationQuaternion(yaw, pitch, roll);
	}

	void TransformComponent::SetRotation(const Quaternion& q)
	{
		m_rotation = q;
	}

	Vector3 TransformComponent::GetEulerRotation()
	{
		return m_rotation.ToEuler();
	}

	Quaternion TransformComponent::GetRotation()
	{
		return m_rotation;
	}

	void TransformComponent::Rotate(const float& yaw, const float& pitch, const float roll)
	{
		m_rotation *= createRotationQuaternion(yaw, pitch, roll);
		m_rotation.Normalize();
	}

	void TransformComponent::Rotate(const Quaternion& q)
	{
		m_rotation *= q;
		m_rotation.Normalize();
	}

	void TransformComponent::RotateRoll(const float& degree)
	{
		m_rotation *= createRotationQuaternion(0, 0, degree);
		m_rotation.Normalize();
	}

	void TransformComponent::RotatePitch(const float& degree)
	{
		m_rotation *= createRotationQuaternion(0, degree, 0);
		m_rotation.Normalize();
	}

	void TransformComponent::RotateYaw(const float& degree)
	{
		m_rotation *= createRotationQuaternion(degree, 0, 0);
		m_rotation.Normalize();
	}

	Vector3 TransformComponent::GetForwardDir()
	{
		return Vector3::Transform(m_localForward, Matrix::CreateFromQuaternion(m_rotation));
	}

	Vector3 TransformComponent::GetRightDir()
	{
		return Vector3::Transform(m_localRight, Matrix::CreateFromQuaternion(m_rotation));
	}

	Vector3 TransformComponent::GetUpDir()
	{
		return Vector3::Transform(m_localUp, Matrix::CreateFromQuaternion(m_rotation));
	}

	Matrix TransformComponent::GetTransformMatrix()
	{
		return Matrix::CreateScale(m_scale) *
			Matrix::CreateFromQuaternion(m_rotation) *
			Matrix::CreateTranslation(m_pos);
	}

	Matrix TransformComponent::GetInvTransformMatrix()
	{
		Quaternion invRotation = m_rotation;
		invRotation.Conjugate();

		Vector3 invScale(
			m_scale.x != 0.0f ? 1.0f / m_scale.x : 0.0f,
			m_scale.y != 0.0f ? 1.0f / m_scale.y : 0.0f,
			m_scale.z != 0.0f ? 1.0f / m_scale.z : 0.0f
		);
		return Matrix::CreateTranslation(-m_pos) * 
			Matrix::CreateFromQuaternion(invRotation) *
			Matrix::CreateScale(invScale);
	}

	Quaternion TransformComponent::createRotationQuaternion(const float& yaw, const float& pitch, const float roll)
	{
		return Quaternion::CreateFromYawPitchRoll(
			XMConvertToRadians(yaw),
			XMConvertToRadians(pitch),
			XMConvertToRadians(roll)
		);
	}

	void TransformComponent::SetBoundingVolumeScale()
	{
		BoundComponent* bound = static_cast<Actor*>(GetOwner())->GetComponent<BoundComponent>();
		if (bound) {
			bound->SetScale(m_scale);
		}
	}
}