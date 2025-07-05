#include "pch.h"
#include "TransformComponent.h"
#include "Actor.h"
#include "BoundComponent.h"

namespace DE {
	using namespace DirectX;

	void TransformComponent::SetRotation(const float& yaw, const float& pitch, const float roll)
	{
		m_localRotation = createRotationQuaternion(yaw, pitch, roll);
	}

	void TransformComponent::SetRotation(const Quaternion& q)
	{
		m_localRotation = q;
		m_localRotation.Normalize();
	}

	void TransformComponent::LocalRotate(const float& yaw, const float& pitch, const float roll)
	{
		m_localRotation *= createRotationQuaternion(yaw, pitch, roll);
		m_localRotation.Normalize();
	}

	void TransformComponent::LocalRotate(const Quaternion& q)
	{
		m_localRotation *= q;
		m_localRotation.Normalize();
	}

	void TransformComponent::Rotate(const float& yaw, const float& pitch, const float roll)
	{
		m_worldRotation += Vector3(yaw, pitch, roll);
		m_worldRotation.x = fmod(m_worldRotation.x, 360.f); // yaw (x)
		m_worldRotation.y = fmod(m_worldRotation.y, 360.f); // pitch (y)
		m_worldRotation.z = fmod(m_worldRotation.z, 360.f); // roll (z)	
	}

	Vector3 TransformComponent::GetForwardDir()
	{
		Quaternion worldRotate = createRotationQuaternion(m_worldRotation.x, m_worldRotation.y, m_worldRotation.z);
		return Vector3::Transform(m_localForward, Matrix::CreateFromQuaternion(worldRotate * m_localRotation));
	}

	Vector3 TransformComponent::GetRightDir()
	{
		Quaternion worldRotate = createRotationQuaternion(m_worldRotation.x, m_worldRotation.y, m_worldRotation.z);
		return Vector3::Transform(m_localRight, Matrix::CreateFromQuaternion(worldRotate * m_localRotation));
	}

	Vector3 TransformComponent::GetUpDir()
	{
		Quaternion worldRotate = createRotationQuaternion(m_worldRotation.x, m_worldRotation.y, m_worldRotation.z);
		return Vector3::Transform(m_localUp, Matrix::CreateFromQuaternion(worldRotate * m_localRotation));
	}

	Matrix TransformComponent::GetTranslateMatrix()
	{
		return Matrix::CreateTranslation(m_pos);
	}

	Matrix TransformComponent::GetRotationMatrix()
	{
		Quaternion worldRotate = createRotationQuaternion(m_worldRotation.x, m_worldRotation.y, m_worldRotation.z);
		Quaternion finalRotation = worldRotate * m_localRotation;

		return Matrix::CreateFromQuaternion(finalRotation);
	}

	Matrix TransformComponent::GetTransformMatrix()
	{
		Quaternion worldRotate = createRotationQuaternion(m_worldRotation.x, m_worldRotation.y, m_worldRotation.z);
		Quaternion finalRotation = worldRotate * m_localRotation;

		return Matrix::CreateScale(m_scale) *
			Matrix::CreateFromQuaternion(finalRotation) *
			Matrix::CreateTranslation(m_pos);
	}

	Matrix TransformComponent::GetInvTransformMatrix()
	{
		Matrix invMat = GetTransformMatrix();
		return invMat.Invert();
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