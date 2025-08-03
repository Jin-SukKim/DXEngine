#include "pch.h"
#include "PointLight.h"
#include "TransformComponent.h"

namespace DE {
	PointLight::PointLight(const std::wstring& name) : Super(name)
	{
		m_light.radiance = Vector3(5.0f);
		m_light.spotPower = 10.0f;
		m_light.fallOffStart = 0.0f;
		m_light.fallOffEnd = 100.0f;
		m_light.haloRadius = 0.5f;
		m_light.haloStrength = 1.0f;
		m_light.type = LIGHT_POINT | LIGHT_SHADOW;

		if (m_light.type & LIGHT_SHADOW) {
			m_lightFov = 91.f;
		}
	}

	void PointLight::Initialize()
	{
		Super::Initialize();

		TransformComponent* tr = GetComponent<TransformComponent>();
		if (tr) {
			tr->SetPos(Vector3(0.f, 1.f, -2.f));
			tr->SetScale(Vector3(0.02f));
		}
	}
	void PointLight::Update(const float& deltaTime)
	{
		if (m_light.type & LIGHT_OFF)
			return; // 빛이 꺼져있으면 업데이트하지 않음

		Actor::Update(deltaTime);

		TransformComponent* tr = GetComponent<TransformComponent>();
		if (tr) {
			m_light.position = tr->GetPos();
			m_light.direction = tr->GetForwardDir();
			m_light.radius = tr->GetScale().x;
		}
	}

	void PointLight::RenderShadow(const std::vector<std::vector<std::shared_ptr<Actor>>>& actorLists)
	{
		if (m_light.type & LIGHT_OFF)
			return; // 빛이 꺼져있으면 렌더링하지 않음

		if (m_light.type & LIGHT_SHADOW) {
			GET_SINGLE(RenderBase)->SetShadowViewport(GetShadowWidth(), GetShadowHeight());
			
			Matrix proj = GetLightProjMatrix();
			
			// 6개의 면에 대해 그림자 맵을 렌더링
			for (int i = 0; i < 6; ++i) {
				GET_SINGLE(RenderBase)->SetShadowMap(m_lightID + i);

				Matrix view = GetCubemapViewMatrix(i);
				UpdateShadowGlobals(view, proj, i);

				SetGlobals(m_shadowGlobalConsts.Get());

				for (const auto& actorList : actorLists)
					for (const auto& actor : actorList)
						if (actor->IsCastShadow() && actor->IsVisible())
							actor->Render();
			}
		}
	}

	void PointLight::UpdateShadowGlobals(const Matrix& view, const Matrix& proj, const int& i)
	{
		m_shadowGlobalConsts.GetCpu().eyeWorld = m_light.position;
		m_shadowGlobalConsts.GetCpu().view = view.Transpose();
		m_shadowGlobalConsts.GetCpu().proj = proj.Transpose();
		m_shadowGlobalConsts.GetCpu().invProj = proj.Invert().Transpose();
		m_shadowGlobalConsts.GetCpu().viewProj = (view * proj).Transpose();

		// 그림자를 실제로 렌더링할 때 필요
		m_light.viewProj[i] = m_shadowGlobalConsts.GetCpu().viewProj;
		m_light.invProj = m_shadowGlobalConsts.GetCpu().invProj;

		m_light.nearPlane = m_nearZ;
		m_light.frustumWidth = GetLightFrustumWidth(proj);

		m_shadowGlobalConsts.Upload();
	}

	Matrix PointLight::GetCubemapViewMatrix(int idx)
	{
		// 각 면이 바라보는 방향(LookAt)과 상향 벡터(Up)
		static const Vector3 lookDir[6] = {
			Vector3(1.0f, 0.0f, 0.0f),   // +X
			Vector3(-1.0f, 0.0f, 0.0f),  // -X
			Vector3(0.0f, 1.0f, 0.0f),   // +Y
			Vector3(0.0f, -1.0f, 0.0f),  // -Y
			Vector3(0.0f, 0.0f, 1.0f),   // +Z
			Vector3(0.0f, 0.0f, -1.0f)   // -Z
		};

		static const Vector3 upDir[6] = {
			Vector3(0.0f, 1.0f, 0.0f),   // +X
			Vector3(0.0f, 1.0f, 0.0f),   // -X
			Vector3(0.0f, 0.0f, -1.0f),  // +Y
			Vector3(0.0f, 0.0f, 1.0f),   // -Y
			Vector3(0.0f, 1.0f, 0.0f),   // +Z
			Vector3(0.0f, 1.0f, 0.0f)    // -Z
		};

		return DirectX::XMMatrixLookAtLH(m_light.position, 
			m_light.position + lookDir[idx], 
			upDir[idx]
		);
	}
}