#include "pch.h"
#include "CameraActor.h"
#include "TransformComponent.h"
#include "AppBase.h"
namespace DE {
    void CameraActor::Initialize()
    {
        m_forward = InputAxisAction(w, s);
        m_right = InputAxisAction(d, a);
        m_up = InputAxisAction(q, e);
    }

    void CameraActor::Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime)
    {
        if (!m_fpv)
            return;

        TransformComponent* tr = this->GetComponent<TransformComponent>();
        if (tr) {
            Vector3 dir = { 0.f, 0.f, 0.f };
            dir += m_forward.GetAxisInput() * tr->GetForwardDir();
            dir += m_right.GetAxisInput() * tr->GetRightDir();
            dir += m_up.GetAxisInput() * tr->GetUpDir();
            dir.Normalize();

            Vector3 pos = tr->GetPos();
            pos += m_speed * dir * AppBase::GetDeltaTime();
            tr->SetPos(pos);

            Vector2 currentMousePos = AppBase::GetInputManager().GetMouseNDC();
            // 좌우 360도, 위 아래 90도
            currentMousePos *= Vector2(DirectX::XM_2PI, -DirectX::XM_PIDIV2) * m_rotateSpeed;
            tr->SetRotation(currentMousePos.x, currentMousePos.y, 0.f);

            //Vector2 mouseDelta = currentMousePos - m_prevMousePos;
            //// 좌우 360도, 위 아래 90도
            //Vector2 rotationDelta = mouseDelta * Vector2(DirectX::XM_2PI, -DirectX::XM_PIDIV2) * m_rotateSpeed;

            //Vector3 euler = tr->GetRotation();
            //euler += Vector3(rotationDelta);
            //tr->SetRotation(euler.x, euler.y, euler.z);

            //m_prevMousePos = currentMousePos;
        }
    }

    Matrix CameraActor::GetViewMatrix()
    {
        TransformComponent* tr = GetComponent<TransformComponent>();
        if (tr)
            return tr->GetInvTransformMatrix();
        return Matrix::Identity;
    }

    Matrix CameraActor::GetProjMatrix()
    {
        return m_usePerspectiveProjection
            // 원근 투영
            ? DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(m_projFovAngleY),
                m_aspect, m_nearZ, m_farZ)
            // 정투영
            : DirectX::XMMatrixOrthographicOffCenterLH(-m_aspect, m_aspect, -1.0f,
                1.0f, m_nearZ, m_farZ);
    }

    Vector3 CameraActor::GetPos() {
        TransformComponent* tr = GetComponent<TransformComponent>();
        if (tr)
            return tr->GetPos();

        return Vector3::Zero;
    }
}