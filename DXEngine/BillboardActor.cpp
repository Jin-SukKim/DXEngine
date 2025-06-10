#include "pch.h"
#include "BillboardActor.h"
#include "ModelComponent.h"
#include "BoundComponent.h"
#include "D3D11Utils.h"
#include "MeshData.h"
#include "RenderBase.h"

namespace DE {
	BillboardActor::BillboardActor(ComPtr<ID3D11Device>& device, const std::wstring& name) : Super(device, name)
	{	
		m_billboardModel = AddComponent<ModelComponent>(device, L"BillboardModel");
		m_billboardBounds = AddComponent<BoundComponent>(device, L"BillboardBound");
	}

	void BillboardActor::Initialize()
	{
		Super::Initialize();

	}

	void BillboardActor::Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime)
	{
		Super::Update(context, deltaTime);
	}

	void BillboardActor::Render(RenderBase& renderer) {
		ComPtr<ID3D11DeviceContext>& context = renderer.GetContext();

		renderer.SetPipelineState(RenderBase::graphicsCommon.billboard.solidPSO);
		context->VSSetConstantBuffers(3, 1, m_billboardConsts.GetAddressOf());
		context->GSSetConstantBuffers(3, 1, m_billboardConsts.GetAddressOf());
		context->PSSetConstantBuffers(3, 1, m_billboardConsts.GetAddressOf());
		m_billboardModel->RenderPoints(context);
		context->GSSetShader(nullptr, 0, 0);

		renderer.SetPipelineState(RenderBase::graphicsCommon.basic.boundPSO);
		RenderComponent(context, ComponentType::BoundingVolume);
	}

	void BillboardActor::SetBillboard(ComPtr<ID3D11Device>& device, const std::vector<Vector3>& points, const float& width, const ComPtr<ID3D11PixelShader>& pixelShader)
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

		m_billboardModel->SetModel(device, meshData);
		m_pixelShader = pixelShader;

		m_billboardBounds->SetBoundingVolume(device, { meshData });

		m_billboardConsts.GetCpu().widthWorld = width;
		m_billboardConsts.Initialize(device);
	}
}

