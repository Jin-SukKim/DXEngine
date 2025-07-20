#include "pch.h"
#include "TransformComponent.h"
#include "Actor.h"
#include "BoundComponent.h"

namespace DE {
	using namespace DirectX;

	void TransformComponent::SetLocalRotation(const float& yaw, const float& pitch, const float roll)
	{
		m_localRotation = Vector3(yaw, pitch, roll);
		m_localQuaternion = createRotationQuaternion(yaw, pitch, roll);
	}

	void TransformComponent::SetLocalRotation(const Quaternion& q)
	{
		m_localQuaternion = q;
		m_localQuaternion.Normalize();

		m_localRotation = m_localQuaternion.ToEuler();
	}

	void TransformComponent::SetRotation(const float& yaw, const float& pitch, const float roll)
	{
		m_worldRotation = Vector3(yaw, pitch, roll);
		m_worldQuaternion = createRotationQuaternion(yaw, pitch, roll);
	}

	void TransformComponent::SetRotation(const Quaternion& q)
	{
		m_worldQuaternion = q;
		m_worldQuaternion.Normalize();

		m_worldRotation = m_worldQuaternion.ToEuler();
	}

	void TransformComponent::Rotate(const float& yaw, const float& pitch, const float roll)
	{
		m_worldRotation += Vector3(yaw, pitch, roll);
		m_worldRotation.x = fmod(m_worldRotation.x, 360.f); // yaw (x)
		m_worldRotation.y = fmod(m_worldRotation.y, 360.f); // pitch (y)
		m_worldRotation.z = fmod(m_worldRotation.z, 360.f); // roll (z)	

		SetRotation(m_worldRotation.x, m_worldRotation.y, m_worldRotation.z);
	}

	Vector3 TransformComponent::GetForwardDir()
	{
		return Vector3::Transform(m_localForward, Matrix::CreateFromQuaternion(m_worldQuaternion * m_localQuaternion));
	}

	Vector3 TransformComponent::GetRightDir()
	{
		return Vector3::Transform(m_localRight, Matrix::CreateFromQuaternion(m_worldQuaternion * m_localQuaternion));
	}

	Vector3 TransformComponent::GetUpDir()
	{
		return Vector3::Transform(m_localUp, Matrix::CreateFromQuaternion(m_worldQuaternion * m_localQuaternion));
	}

	Matrix TransformComponent::GetTranslateMatrix()
	{
		return Matrix::CreateTranslation(m_pos);
	}

	Matrix TransformComponent::GetRotationMatrix()
	{
		Quaternion finalRotation = m_worldQuaternion * m_localQuaternion;

		return Matrix::CreateFromQuaternion(finalRotation);
	}

	Matrix TransformComponent::GetTransformMatrix()
	{
		Quaternion finalRotation = m_worldQuaternion * m_localQuaternion;

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
		Quaternion rot = Quaternion::CreateFromYawPitchRoll(
			XMConvertToRadians(yaw),
			XMConvertToRadians(pitch),
			XMConvertToRadians(roll)
		);
		rot.Normalize();
		return rot;
	}

	void TransformComponent::SetBoundingVolumeScale()
	{
		BoundComponent* bound = static_cast<Actor*>(GetOwner())->GetComponent<BoundComponent>();
		if (bound) {
			bound->SetScale(m_scale);
		}
	}
}