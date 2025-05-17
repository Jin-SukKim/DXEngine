#include "pch.h"
#include "Scene.h"

#include "GeometryGenerator.h"
#include "D3D11Utils.h"


namespace DE {
	Scene::Scene(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context) : m_device(device), m_context(context)
	{
	}

	void Scene::Initialize() {
		MeshData meshData = GeometryGenerator::MakeTriangle();
		triangle.indexCount = UINT(meshData.indices.size());
		D3D11Utils::CreateVertexBuffer(m_device, meshData.vertices, triangle.vertexBuffer);
		D3D11Utils::CreateIndexBuffer(m_device, meshData.indices, triangle.indexBuffer);

		constantData.world = Matrix();
		constantData.view = Matrix();
		constantData.proj = Matrix();
		D3D11Utils::CreateConstantBuffer(m_device, constantData, triangle.meshConstBuffer);

		std::vector<D3D11_INPUT_ELEMENT_DESC> inputElements = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 6, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};
		D3D11Utils::CreateVSAndIL(m_device, L"BasicVS.hlsl", inputElements, vs, il);
		D3D11Utils::CreatePS(m_device, L"BasicPS.hlsl", ps);
	}

	void Scene::Update() {
		// constant buffer data °»½Å

		// Constant Data¸¦ CPU -> GPU
		D3D11Utils::UpdateBuffer(m_context, constantData, triangle.meshConstBuffer);
	}

	void Scene::Render() {

		m_context->VSSetShader(vs.Get(), 0, 0);
		m_context->VSSetConstantBuffers(0, 1, triangle.meshConstBuffer.GetAddressOf());
		m_context->PSSetShader(ps.Get(), 0, 0);


		UINT stride = sizeof(Vertex);
		UINT offset = 0;

		m_context->IASetInputLayout(il.Get());
		m_context->IASetVertexBuffers(0, 1, triangle.vertexBuffer.GetAddressOf(), &stride, &offset);
		m_context->IASetIndexBuffer(triangle.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_context->DrawIndexed(triangle.indexCount, 0, 0);
	}
}