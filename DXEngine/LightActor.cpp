#include "pch.h"
#include "LightActor.h"
#include "TransformComponent.h"
#include "RenderBase.h"

namespace DE {
	UINT LightActor::lightID = 0;

	LightActor::LightActor(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const std::wstring& name) : Super(device, context, name), m_lightID(lightID++)
	{
		m_light.radiance = Vector3(5.0f);
		m_light.spotPower = 10.0f;                      // 좀 더 집중된 빛
		m_light.fallOffStart = 0.0f;
		m_light.fallOffEnd = 100.0f;
		m_light.type = LIGHT_OFF;

		m_shadowGlobalConsts.Initialize(device);
	}

	void LightActor::Initialize()
	{
		Super::Initialize();

		//TransformComponent* tr = GetComponent<TransformComponent>();
		//if (tr) {
		//	tr->SetPos(Vector3(2.0f, 1.f, -1.0f));
		//	tr->SetRotation(-90.f, 45.f, 0.f);
		//	tr->SetScale(Vector3(0.02f));
		//}
	}

	void LightActor::Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime)
	{
		if (m_light.type & LIGHT_OFF)
			return; // 빛이 꺼져있으면 업데이트하지 않음
		Super::Update(context, deltaTime);

		TransformComponent* tr = GetComponent<TransformComponent>();
		if (tr) {
			m_light.position = tr->GetPos();
			m_light.direction = tr->GetForwardDir();
			m_light.radius = tr->GetScale().x;
		}

		// 그림자맵을 만들기 위한 시점
		if (m_light.type & LIGHT_SHADOW) {
			Matrix view = GetLightViewMatrix();
			Matrix proj = GetLightProjMatrix();

			UpdateShadowGlobals(context, view, proj);
		}
	}

	void LightActor::Render(RenderBase& renderer)
	{
		if (m_light.type & LIGHT_OFF)
			return; // 빛이 꺼져있으면 렌더링하지 않음

		Super::Render(renderer);
		// TODO: 빛을 의미하는 표현하는 걸 렌더링하면 좋을듯 (언리얼 엔진 생각해보기)
	}

	void LightActor::RenderShadow(RenderBase& renderer, const std::vector<std::shared_ptr<Actor>>& actorList)
	{
		if (m_light.type & LIGHT_OFF)
			return; // 빛이 꺼져있으면 렌더링하지 않음

		if (m_light.type & LIGHT_SHADOW) {
			SetGlobals(renderer.GetContext(), m_shadowGlobalConsts.Get());

			for (const auto& actor : actorList) {
				if (actor->IsCastShadow() && actor->IsVisible()) {
					actor->Render(renderer);
				}
			}
		}
	}

	void LightActor::UpdateShadowGlobals(ComPtr<ID3D11DeviceContext>& context, const Matrix& view, const Matrix& proj)
	{
		m_shadowGlobalConsts.GetCpu().eyeWorld = m_light.position;
		m_shadowGlobalConsts.GetCpu().view = view.Transpose();
		m_shadowGlobalConsts.GetCpu().proj = proj.Transpose();
		m_shadowGlobalConsts.GetCpu().invProj = proj.Invert().Transpose();
		m_shadowGlobalConsts.GetCpu().viewProj = (view * proj).Transpose();

		// 그림자를 실제로 렌더링할 때 필요
		m_light.viewProj = m_shadowGlobalConsts.GetCpu().viewProj;
		m_light.invProj = m_shadowGlobalConsts.GetCpu().invProj;

		m_light.nearPlane = m_nearZ;
		m_light.frustumWidth = GetLightFrustumWidth(proj);

		m_shadowGlobalConsts.Upload(context);
	}

	float LightActor::GetLightFrustumWidth(const Matrix& proj)
	{
		// LIGHT_FRUSTUM_WIDTH 확인
		// Screen의 왼쪽 가장자리와 오른쪽 가장자리 좌표를 Projection 행렬에 대해 역변환
		// 그러면 왼쪽 가장자리와 오른쪽 가장자리의 View 좌표계에서의 좌표를 구할 수 있음
		Vector4 xLeft(-1.0f, -1.0f, 0.0f, 1.0f);
		Vector4 xRight(1.0f, 1.0f, 0.0f, 1.0f);
		xLeft = Vector4::Transform(xLeft, proj.Invert());
		xRight = Vector4::Transform(xRight, proj.Invert());
		xLeft /= xLeft.w;
		xRight /= xRight.w;
		// 두 좌표값의 x값 차이를 구해 View 좌표계에서 LIght의 ViewFrustum의 Width를 계산
		// LIGHT_FRUSTUM_WIDTH
		return xRight.x - xLeft.x;
	}

	Matrix LightActor::GetLightViewMatrix()
	{
		TransformComponent* tr = GetComponent<TransformComponent>();
		if (tr)
			//return tr->GetInvTransformMatrix(); // Camera의 View Matrix 방식과 똑같이 구현하면 안되는데 아직 이유를 잘 모르겠음
			return DirectX::XMMatrixLookAtLH(m_light.position, m_light.position + m_light.direction, tr->GetUpDir());
		return Matrix();
	}

	Matrix LightActor::GetLightProjMatrix()
	{	
		// Light의 FOV는 빛이 어디까지 비출지를 결정 (빛이 비추는 범위로 그림자 생성에 영향을 줌)
		return DirectX::XMMatrixPerspectiveFovLH(
			DirectX::XMConvertToRadians(m_lightFov), m_aspectRatio, m_nearZ, m_farZ);
	}

	void LightActor::SetGlobals(ComPtr<ID3D11DeviceContext>& context, const ComPtr<ID3D11Buffer>& globalConstsGPU) {
		// Global Constants을 Shader에서 사용할 수 있도록 설정
		context->VSSetConstantBuffers(0, 1, globalConstsGPU.GetAddressOf());
		context->GSSetConstantBuffers(0, 1, globalConstsGPU.GetAddressOf());
		context->PSSetConstantBuffers(0, 1, globalConstsGPU.GetAddressOf());
	}
}