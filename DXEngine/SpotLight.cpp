#include "pch.h"
#include "SpotLight.h"
#include "TransformComponent.h"

namespace DE {

	SpotLight::SpotLight(const std::wstring& name) : Super(name)
	{
		m_light.radiance = Vector3(5.0f);
		m_light.spotPower = 10.0f;    
		m_light.fallOffStart = 0.0f;
		m_light.fallOffEnd = 100.0f;
		m_light.haloRadius = 0.5f;
		m_light.haloStrength = 1.0f;
		m_light.type = LIGHT_SPOT | LIGHT_SHADOW;
	}

	void SpotLight::Initialize()
	{
		Super::Initialize();

		TransformComponent* tr = GetComponent<TransformComponent>();
		if (tr) {
			tr->SetPos(Vector3(2.0f, 1.f, -1.0f));
			tr->SetRotation(-90.f, 45.f, 0.f);
			tr->SetScale(Vector3(0.02f));
		}
	}

	void SpotLight::Update(const float& deltaTime)
	{
		if (m_light.type & LIGHT_OFF)
			return; // 빛이 꺼져있으면 업데이트하지 않음
		Super::Update(deltaTime);
	}

	float SpotLight::GetLightFrustumWidth(const Matrix& proj)
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

}