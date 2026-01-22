#include "pch.h"
#include "TransformComponent.h"
#include "Actor.h"
#include "BoundComponent.h"

namespace DE {
	using namespace DirectX;
	
	void TransformComponent::Initialize()
	{
		Super::Initialize();
		m_constant.Initialize();
	}

	void TransformComponent::Update(const float& deltaTime)
	{
		Super::Update(deltaTime);
		Matrix world = this->GetTransformMatrix();
		m_constant.GetCpu().world = world.Transpose();
		world.Translation(Vector3(0.f));
		world = world.Invert().Transpose();
		m_constant.GetCpu().worldIT = world.Transpose();

		m_constant.Upload();
	}

	void TransformComponent::Render()
	{
		Super::Render();
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();
		context->VSSetConstantBuffers(1, 1, m_constant.GetAddressOf());
		context->PSSetConstantBuffers(1, 1, m_constant.GetAddressOf());
		context->CSSetConstantBuffers(1, 1, m_constant.GetAddressOf());
	}

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

	void TransformComponent::SetRotation(const Vector3& rot)
	{
		m_worldRotation = rot;
		m_worldQuaternion = createRotationQuaternion(rot.x, rot.y, rot.z);
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
		Vector3 forward = Vector3::Transform(m_localForward, Matrix::CreateFromQuaternion(m_worldQuaternion * m_localQuaternion));
		forward.Normalize();
		return forward;
	}

	Vector3 TransformComponent::GetRightDir()
	{
		Vector3 right = Vector3::Transform(m_localRight, Matrix::CreateFromQuaternion(m_worldQuaternion * m_localQuaternion));
		right.Normalize();
		return right;
	}

	Vector3 TransformComponent::GetUpDir()
	{
		Vector3 up = Vector3::Transform(m_localUp, Matrix::CreateFromQuaternion(m_worldQuaternion * m_localQuaternion));
		up.Normalize();
		return up;
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

	void TransformComponent::UpdateGui()
	{
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode("Transform")) {
			pos[0] = m_pos.x;
			pos[1] = m_pos.y;
			pos[2] = m_pos.z;

			rotate[0] = m_worldRotation.x;
			rotate[1] = m_worldRotation.y;
			rotate[2] = m_worldRotation.z;

			scale[0] = m_scale.x;
			scale[1] = m_scale.y;
			scale[2] = m_scale.z;

			ImGui::DragFloat3("Location", pos, 0.001f, -10000.f, 10000.f);
			SetPos(Vector3(pos[0], pos[1], pos[2]));

			ImGui::DragFloat3("Rotation", rotate, 0.001f, -10000.f, 10000.f);
			SetRotation(rotate[0], rotate[1], rotate[2]);

			ImGui::DragFloat3("Scale", scale, 0.001f, -10000.f, 10000.f);
			SetScale(Vector3(scale[0], scale[1], scale[2]));

			ImGui::TreePop();
		}
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