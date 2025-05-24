#include "pch.h"
#include "CameraActor.h"
#include "TransformComponent.h"

namespace DE {

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