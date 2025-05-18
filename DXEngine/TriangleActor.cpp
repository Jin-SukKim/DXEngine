#include "pch.h"
#include "TriangleActor.h"

#include "GeometryGenerator.h"
#include "D3D11Utils.h"
#include "TransformComponent.h"

namespace DE {
	void TriangleActor::Initialize(ComPtr<ID3D11Device>& device) {
		MeshData meshData = GeometryGenerator::MakeTriangle();
		triangle.indexCount = UINT(meshData.indices.size());
		D3D11Utils::CreateVertexBuffer(device, meshData.vertices, triangle.vertexBuffer);
		D3D11Utils::CreateIndexBuffer(device, meshData.indices, triangle.indexBuffer);

		D3D11Utils::CreateConstantBuffer(device, m_constantCPU, triangle.meshConstGPU);

		std::vector<D3D11_INPUT_ELEMENT_DESC> inputElements = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 6, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};
		D3D11Utils::CreateVSAndIL(device, L"BasicVS.hlsl", inputElements, vs, il);
		D3D11Utils::CreatePS(device, L"BasicPS.hlsl", ps);
	}

	void TriangleActor::Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime) {
		// constant buffer data °»½Å
		TransformComponent* tr = this->GetComponent<TransformComponent>();
		if (tr) {
			m_constantCPU.world = tr->GetTransformMatrix().Transpose();
		}

		// Constant Data¸¦ CPU -> GPU
		D3D11Utils::UpdateBuffer(context, m_constantCPU, triangle.meshConstGPU);
	}

	void TriangleActor::Render(ComPtr<ID3D11DeviceContext>& context) {
		context->VSSetShader(vs.Get(), 0, 0);
		context->VSSetConstantBuffers(1, 1, triangle.meshConstGPU.GetAddressOf());
		context->PSSetShader(ps.Get(), 0, 0);


		UINT stride = sizeof(Vertex);
		UINT offset = 0;

		context->IASetInputLayout(il.Get());
		context->IASetVertexBuffers(0, 1, triangle.vertexBuffer.GetAddressOf(), &stride, &offset);
		context->IASetIndexBuffer(triangle.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context->DrawIndexed(triangle.indexCount, 0, 0);
	}
}