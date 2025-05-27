#include "pch.h"
#include "ModelComponent.h"
#include "GeometryGenerator.h"
#include "TransformComponent.h"
#include "Actor.h"

namespace DE {
	void ModelComponent::SetModel(ComPtr<ID3D11Device>& device, const std::wstring& name, const std::string& basePath, const std::string& filename)
	{
		std::vector<MeshData> meshes = GeometryGenerator::ReadFromFile(basePath, filename);
		SetModel(device, meshes);
	}

	void ModelComponent::SetModel(ComPtr<ID3D11Device>& device, const std::vector<MeshData>& meshes)
	{
		// 일반적으로는 각 Mesh가 각각의 mesh/materialConsts를 각자 가질 수 있는데 여기서는 하나의 Constant Buffer를 공유
		m_constant.Initialize(device);
		m_basicMaterial.Initialize(device);

		for (const auto& meshData : meshes) {
			Mesh newMesh;
			D3D11Utils::CreateVertexBuffer(device, meshData.vertices, newMesh.vertexBuffer);
			D3D11Utils::CreateIndexBuffer(device, meshData.indices, newMesh.indexBuffer);
			
			newMesh.indexCount = UINT(meshData.indices.size());
			newMesh.vertexCount = UINT(meshData.vertices.size());
			newMesh.stride = UINT(sizeof(Vertex));
			
			if (!meshData.albedoTextureFilename.empty()) {
				std::cout << meshData.albedoTextureFilename << std::endl;
				D3D11Utils::CreateTexture(device, meshData.albedoTextureFilename, newMesh.albedoTexture);
			}

			// 모델의 모든 Mesh가 같은 Buffer를 사용
			newMesh.meshConstGPU = m_constant.Get();
			newMesh.basicMaterialConstGPU = m_basicMaterial.Get();

			m_meshes.emplace_back(newMesh);
		}
	}

	void ModelComponent::Initialize() {
		Super::Initialize();
	}
	
	void ModelComponent::Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime) {
		Super::Update(context, deltaTime);
		// Constant Data를 CPU -> GPU
		if (m_meshes.empty())
			return;

		if (updateWorldCpu()) {
			// 현재 모델의 모든 Mesh가 buffer를 공유하기 때문에 하나만 복사
			m_constant.Upload(context);
			m_basicMaterial.Upload(context);
		}
	}

	bool ModelComponent::updateWorldCpu()
	{
		Actor* owner = dynamic_cast<Actor*>(GetOwner());
		if (!owner)
			return false;

		TransformComponent* tr = owner->GetComponent<TransformComponent>();
		if (tr) {
			Matrix world = tr->GetTransformMatrix();
			m_constant.GetCpu().world = world.Transpose();
			world.Translation(Vector3(0.f));
			world = world.Invert().Transpose();
			m_constant.GetCpu().worldIT = world.Transpose();
		}

		return true;
	}

	void ModelComponent::Render(ComPtr<ID3D11DeviceContext>& context) {
		Super::Render(context);
		for (const auto& mesh : m_meshes) {
			ID3D11Buffer* constBuffers[2] = {
				mesh.basicMaterialConstGPU.Get(),
				mesh.meshConstGPU.Get()
			};
			context->VSSetConstantBuffers(1, 2, constBuffers);

			ID3D11ShaderResourceView* resViews[1] = { mesh.albedoTexture.GetSRV() };
			context->PSSetShaderResources(0, 1, resViews);
			context->PSSetConstantBuffers(1, 2, constBuffers);
			context->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &mesh.stride, &mesh.offset);
			context->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			context->DrawIndexed(mesh.indexCount, 0, 0);
		}
	}

	void ModelComponent::RenderNormal(ComPtr<ID3D11DeviceContext>& context)
	{
		if (!m_drawNormal)
			return;
		// Normal Vector 그리기
		for (const auto& mesh : m_meshes) {
			ID3D11Buffer* constBuffers[2] = {mesh.basicMaterialConstGPU.Get(),
											 mesh.meshConstGPU.Get()};
			context->GSSetConstantBuffers(1, 2, constBuffers);
			context->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &mesh.stride, &mesh.offset);
			context->Draw(mesh.vertexCount, 0);
		}
	}

}