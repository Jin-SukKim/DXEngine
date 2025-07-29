#include "pch.h"
#include "CameraActor.h"
#include "TransformComponent.h"
#include "AppBase.h"

namespace DE {
    void CameraActor::Initialize()
    {
        m_forward = InputAxisAction(w, s);
        m_right = InputAxisAction(d, a);
        m_up = InputAxisAction(e, q);

        m_mouseClick = InputAxisAction(lButton, rButton);
    }

    void CameraActor::Update(const float& deltaTime)
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
            currentMousePos.y = -currentMousePos.y;

            if (m_mouseClick.GetAxisInput() < 0.f) {
                Vector2 mouseDelta = currentMousePos - m_prevMousePos;
                mouseDelta *= m_rotateSpeed;
                tr->Rotate(mouseDelta.x, mouseDelta.y, 0.f);
            }

            m_prevMousePos = currentMousePos;
        }
    }

    Matrix CameraActor::GetViewMatrix()
    {
        TransformComponent* tr = GetComponent<TransformComponent>();
        if (tr)
            return tr->GetInvTransformMatrix();
        return Matrix();
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

        return Vector3(0.f, 0.f, 0.f);
    }
}