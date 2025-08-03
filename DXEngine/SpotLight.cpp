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

	//void SpotLight::CreateShadowBuffer()
	//{
	//	D3D11_TEXTURE2D_DESC dsDesc;
	//	dsDesc.Width = m_shadowWidth;
	//	dsDesc.Height = m_shadowHeight;
	//	dsDesc.MipLevels = 1; // Depth Stencil Buffer는 Mipmap 불필요
	//	dsDesc.ArraySize = 1;
	//	dsDesc.Usage = D3D11_USAGE_DEFAULT;
	//	dsDesc.CPUAccessFlags = 0;
	//	dsDesc.MiscFlags = 0;
	//	dsDesc.SampleDesc.Count = 1;
	//	dsDesc.SampleDesc.Quality = 0;
	//	dsDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	//	dsDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

	//	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	//	ZeroMemory(&dsvDesc, sizeof(dsvDesc));
	//	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT; // D32 Format 사용
	//	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

	//	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	//	ZeroMemory(&srvDesc, sizeof(srvDesc));
	//	srvDesc.Format = DXGI_FORMAT_R32_FLOAT; // SRV로 사용하기 위해 R32 format 사용
	//	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	//	srvDesc.Texture2D.MipLevels = 1;

	//	ComPtr<ID3D11Device>& device = GET_SINGLE(RenderBase)->GetDevice();
	//	ThrowIfFailed(device->CreateTexture2D(&dsDesc, NULL, m_shadowBuffer.GetAddressOfTexture()));
	//	// Shadow Map용 DSV 생성 (dsvDesc는 위에서 DepthOnlyDSV만들떄 사용한 걸 그대로 사용)
	//	ThrowIfFailed(device->CreateDepthStencilView(m_shadowBuffer.GetTexture(), &dsvDesc, m_shadowDSV.GetAddressOf()));
	//	// Shadow Map용 SRV 생성 (위에서 만든 srvDesc 그대로 사용)
	//	ThrowIfFailed(device->CreateShaderResourceView(m_shadowBuffer.GetTexture(), &srvDesc, m_shadowBuffer.GetAddressOfSRV()));
	//}

}