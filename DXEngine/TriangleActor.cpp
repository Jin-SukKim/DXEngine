#include "pch.h"
#include "TriangleActor.h"

#include "GeometryGenerator.h"
#include "D3D11Utils.h"
#include "TransformComponent.h"

namespace DE {
	void TriangleActor::Initialize(ComPtr<ID3D11Device>& device) {
		MeshData meshData = GeometryGenerator::MakeBox();
		triangle.indexCount = UINT(meshData.indices.size());
		D3D11Utils::CreateVertexBuffer(device, meshData.vertices, triangle.vertexBuffer);
		D3D11Utils::CreateIndexBuffer(device, meshData.indices, triangle.indexBuffer);

		D3D11Utils::CreateConstantBuffer(device, m_constantCPU, triangle.meshConstGPU);
		D3D11Utils::CreateConstantBuffer(device, m_basicMaterialCPU, triangle.basicMaterialConstGPU);

		std::vector<D3D11_INPUT_ELEMENT_DESC> inputElements = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 6, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};
		D3D11Utils::CreateVSAndIL(device, L"BasicVS.hlsl", inputElements, vs, il);
		D3D11Utils::CreatePS(device, L"BasicPS.hlsl", ps);

		// Texture 
		D3D11Utils::CreateTexture(device, "../Assets/Textures/crate2_diffuse.png", triangle.albedoTexture);

		// Texture sampler 만들기
		D3D11_SAMPLER_DESC sampDesc;
		ZeroMemory(&sampDesc, sizeof(sampDesc));
		sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; // Linear Interpolation
		// Wrap
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP; 
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampDesc.MinLOD = 0;
		sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

		// Create the Sample State
		device->CreateSamplerState(&sampDesc, m_samplerState.GetAddressOf());
	}

	void TriangleActor::Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime) {
		// constant buffer data 갱신
		TransformComponent* tr = this->GetComponent<TransformComponent>();
		if (tr) {
			Matrix world = tr->GetTransformMatrix();
			m_constantCPU.world = world.Transpose();
			world.Translation(Vector3(0.f));
			m_constantCPU.worldIT = world.Invert().Transpose();
		}

		// Constant Data를 CPU -> GPU
		D3D11Utils::UpdateBuffer(context, m_constantCPU, triangle.meshConstGPU);
		D3D11Utils::UpdateBuffer(context, m_basicMaterialCPU, triangle.basicMaterialConstGPU);
	}

	void TriangleActor::Render(ComPtr<ID3D11DeviceContext>& context) {
		context->VSSetShader(vs.Get(), 0, 0);
		context->VSSetConstantBuffers(2, 1, triangle.meshConstGPU.GetAddressOf());

		ID3D11ShaderResourceView* pixelResources[1] = { triangle.albedoTexture.GetSRV() };
		context->PSSetShaderResources(0, 1, pixelResources);
		context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());
		context->PSSetConstantBuffers(1, 1, triangle.basicMaterialConstGPU.GetAddressOf());
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