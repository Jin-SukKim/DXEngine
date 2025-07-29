#include "pch.h"
#include "BillboardActor.h"
#include "ModelComponent.h"
#include "BoundComponent.h"
#include "D3D11Utils.h"
#include "MeshData.h"
#include "RenderBase.h"

namespace DE {
	BillboardActor::BillboardActor(const std::wstring& name) : Super(name)
	{	
		m_billboardModel = AddComponent<ModelComponent>(L"BillboardModel");
		m_billboardBounds = AddComponent<BoundComponent>(L"BillboardBound");
	}

	void BillboardActor::Initialize()
	{
		Super::Initialize();

		m_billboardBounds->SetVisibility(false);
	}

	void BillboardActor::Update(const float& deltaTime)
	{
		Super::Update(deltaTime);
		m_billboardConsts.Upload();
	}

	void BillboardActor::Render() {
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		if (m_pixelShader)
			context->PSSetShader(m_pixelShader.Get(), 0, 0);
		context->VSSetConstantBuffers(3, 1, m_billboardConsts.GetAddressOf());
		context->GSSetConstantBuffers(3, 1, m_billboardConsts.GetAddressOf());
		context->PSSetConstantBuffers(3, 1, m_billboardConsts.GetAddressOf());
		
		// Texture Array도 Texture2D와 동일하게 사용
		context->PSSetShaderResources(0, 1, m_texArray.GetAddressOfSRV());

		m_billboardModel->RenderPoints();
		context->GSSetShader(nullptr, 0, 0);
	}

	void BillboardActor::SetBillboard(const std::vector<Vector3>& points, const float& width, const std::vector<std::string>& filenames, const ComPtr<ID3D11PixelShader>& pixelShader)
	{
		MeshData meshData;
		uint32_t indexCount = 0;
		// billboard는 기본적으로 point만 필요
		for (const Vector3& pos : points) {
			Vertex v;
			v.position = pos;

			meshData.vertices.emplace_back(v);
			meshData.indices.emplace_back(indexCount++);
		}

		m_billboardModel->SetModel(meshData);
		m_pixelShader = pixelShader;

		m_billboardBounds->SetBoundingVolume({ meshData });

		if (filenames.size()) {
			ComPtr<ID3D11Device>& device = GET_SINGLE(RenderBase)->GetDevice();
			ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

			D3D11Utils::CreateTextureArray(device, context, filenames, m_texArray);
			m_billboardConsts.GetCpu().arraySize = UINT(filenames.size());
		}

		m_billboardConsts.GetCpu().widthWorld = width;
		m_billboardConsts.Initialize();
	}
}

