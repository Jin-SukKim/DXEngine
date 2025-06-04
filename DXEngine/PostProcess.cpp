#include "pch.h"
#include "PostProcess.h"
#include "GeometryGenerator.h"
#include "GraphicsCommon.h"
#include "Mesh.h"
#include "RenderBase.h"

namespace DE {
	void PostProcess::Initialize(RenderBase& renderer, const std::vector<ComPtr<ID3D11ShaderResourceView>>& resources, const std::vector<ComPtr<ID3D11RenderTargetView>>& targets, int width, int height)
	{
		// 화면을 다 가리는 사각형을 생성하고 이 사각형에 Texture를 Shading해서(PostProcessing) 최종 화면을 렌더링
		MeshData meshData = GeometryGenerator::MakeSquare();

		m_mesh = std::make_shared<Mesh>();
		D3D11Utils::CreateVertexBuffer(renderer.GetDevice(), meshData.vertices, m_mesh->vertexBuffer);
		m_mesh->indexCount = UINT(meshData.indices.size());
		D3D11Utils::CreateIndexBuffer(renderer.GetDevice(), meshData.indices, m_mesh->indexBuffer);
		m_mesh->stride = UINT(sizeof(Vertex));
		m_mesh->offset = 0;
	}

	void PostProcess::Render(RenderBase& renderer) {
		ComPtr<ID3D11DeviceContext>& context = renderer.GetContext();
		context->PSSetSamplers(0, 1, RenderBase::graphicsCommon.linearClampSS.GetAddressOf());

		context->IASetVertexBuffers(0, 1, m_mesh->vertexBuffer.GetAddressOf(), &m_mesh->stride, &m_mesh->offset);
		context->IASetIndexBuffer(m_mesh->indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	}

	void PostProcess::RenderImageFilter(ComPtr<ID3D11DeviceContext>& context, const ImageFilter& imageFilter) {
		imageFilter.Render(context);
		context->DrawIndexed(m_mesh->indexCount, 0, 0);
	}

	void PostProcess::CreateBuffer(ComPtr<ID3D11Device>& device, int width, int height, Texture2D& texture)
	{
		D3D11Utils::CreateImageFilterTexture(device, width, height, texture);
	}

};