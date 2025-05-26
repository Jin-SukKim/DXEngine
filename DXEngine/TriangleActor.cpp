#include "pch.h"
#include "TriangleActor.h"

#include "GeometryGenerator.h"
#include "D3D11Utils.h"
#include "TransformComponent.h"

namespace DE {
	void TriangleActor::Initialize(ComPtr<ID3D11Device>& device) {
		//MeshData meshData = GeometryGenerator::MakeBox();
		//triangle.indexCount = UINT(meshData.indices.size());

		//// Texture 
		//D3D11Utils::CreateTexture(device, "../Assets/Textures/crate2_diffuse.png", triangle.albedoTexture);

		// Main Object
		{
			std::vector<MeshData> meshDataset = GeometryGenerator::ReadFromFile("../Assets/Characters/Zelda/source/", "zeldaPosed001.fbx");
			//std::vector<MeshData> meshDataset = { GeometryGenerator::MakeBox() };

			// 일반적으로는 각 Mesh가 각각의 mesh/materialConsts를 각자 가질 수 있는데 여기서는 하나의 Constant Buffer를 공유
			ComPtr<ID3D11Buffer> meshConstGPU;
			ComPtr<ID3D11Buffer> basicMaterialConstGPU;
			D3D11Utils::CreateConstantBuffer(device, m_constantCPU, meshConstGPU);
			D3D11Utils::CreateConstantBuffer(device, m_basicMaterialCPU, basicMaterialConstGPU);

			for (const auto& meshData : meshDataset) {
				Mesh newMesh;
				D3D11Utils::CreateVertexBuffer(device, meshData.vertices, newMesh.vertexBuffer);
				D3D11Utils::CreateIndexBuffer(device, meshData.indices, newMesh.indexBuffer);
				newMesh.indexCount = UINT(meshData.indices.size());
				newMesh.vertexCount = UINT(meshData.vertices.size());

				if (!meshData.albedoTextureFilename.empty()) {
					std::cout << meshData.albedoTextureFilename << std::endl;
					D3D11Utils::CreateTexture(device, meshData.albedoTextureFilename, newMesh.albedoTexture);
				}

				// 모델의 모든 Mesh가 같은 Buffer를 사용
				newMesh.meshConstGPU = meshConstGPU;
				newMesh.basicMaterialConstGPU = basicMaterialConstGPU;

				m_meshes.emplace_back(newMesh);
			}
		}


		std::vector<D3D11_INPUT_ELEMENT_DESC> inputElements = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 6, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};
		D3D11Utils::CreateVSAndIL(device, L"BasicVS.hlsl", inputElements, vs, il);
		D3D11Utils::CreatePS(device, L"BasicPS.hlsl", ps);

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

		// Normal Vector 렌더링용
		{
			D3D11Utils::CreateVSAndIL(device, L"NormalVS.hlsl", inputElements, normalVS, il);
			D3D11Utils::CreateGS(device, L"NormalGS.hlsl", normalGS);
			D3D11Utils::CreatePS(device, L"NormalPS.hlsl", normalPS);
		}
	}

	void TriangleActor::Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime) {
		// constant buffer data 갱신
		TransformComponent* tr = this->GetComponent<TransformComponent>();
		if (tr) {
			tr->RotateYaw(0.05);
			tr->RotatePitch(0.05);
			Matrix world = tr->GetTransformMatrix();
			m_constantCPU.world = world.Transpose();
			world.Translation(Vector3(0.f));
			world = world.Invert().Transpose();
			m_constantCPU.worldIT = world.Transpose();
		}

		// Constant Data를 CPU -> GPU
		if (!m_meshes.empty()) {
			// 현재 모델의 모든 Mesh가 buffer를 공유하기 때문에 하나만 복사
			D3D11Utils::UpdateBuffer(context, m_constantCPU, m_meshes[0].meshConstGPU);
			D3D11Utils::UpdateBuffer(context, m_basicMaterialCPU, m_meshes[0].basicMaterialConstGPU);
		}
	}

	void TriangleActor::Render(ComPtr<ID3D11DeviceContext>& context) {
		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		for (const auto& mesh : m_meshes) {
			ID3D11Buffer* constBuffers[2] = {
				mesh.basicMaterialConstGPU.Get(),
				mesh.meshConstGPU.Get()
			};
			context->VSSetShader(vs.Get(), 0, 0);
			context->VSSetConstantBuffers(1, 2, constBuffers);

			ID3D11ShaderResourceView* resViews[1] = { mesh.albedoTexture.GetSRV() };
			context->PSSetShaderResources(0, 1, resViews);
			context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());
			context->PSSetConstantBuffers(1, 2, constBuffers);
			context->PSSetShader(ps.Get(), 0, 0);

			context->IASetInputLayout(il.Get());
			context->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
			context->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			context->DrawIndexed(mesh.indexCount, 0, 0);

			// Normal Vector 그리기
			if (m_drawNormal) {
				context->VSSetShader(normalVS.Get(), 0, 0);
				context->PSSetShader(normalPS.Get(), 0, 0);
				context->GSSetConstantBuffers(1, 2, constBuffers);
				context->GSSetShader(normalGS.Get(), 0, 0);
				context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
				context->Draw(mesh.vertexCount, 0);

				context->GSSetShader(nullptr, 0, 0);
			}
		}
	}
}