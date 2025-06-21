#include "pch.h"
#include "ModelComponent.h"
#include "GeometryGenerator.h"
#include "TransformComponent.h"
#include "Actor.h"

#include <filesystem>

namespace DE {
	void ModelComponent::SetModel(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const std::wstring& name, const std::string& basePath, const std::string& filename, bool isGLTF)
	{
		std::vector<MeshData> meshes = GeometryGenerator::ReadFromFile(basePath, filename);
		SetModel(device, context, meshes, isGLTF);
	}

	void ModelComponent::SetModel(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const MeshData& mesh, bool isGLTF)
	{
		SetModel(device, context, std::vector<MeshData>{mesh}, isGLTF);
	}

	void ModelComponent::SetModel(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const std::vector<MeshData>& meshes, bool isGLTF)
	{
		// 일반적으로는 각 Mesh가 각각의 mesh/materialConsts를 각자 가질 수 있는데 여기서는 하나의 Constant Buffer를 공유
		m_constant.Initialize(device);
		m_basicMaterial.Initialize(device);
		m_material.Initialize(device);

		for (const auto& meshData : meshes) {
			Mesh newMesh;
			D3D11Utils::CreateVertexBuffer(device, meshData.vertices, newMesh.vertexBuffer);
			D3D11Utils::CreateIndexBuffer(device, meshData.indices, newMesh.indexBuffer);
			
			newMesh.indexCount = UINT(meshData.indices.size());
			newMesh.vertexCount = UINT(meshData.vertices.size());
			newMesh.stride = UINT(sizeof(Vertex));
			
			if (!meshData.albedoTextureFilename.empty()) {
				std::cout << meshData.albedoTextureFilename << std::endl;
				D3D11Utils::CreateTexture(device, context, meshData.albedoTextureFilename, true, newMesh.albedoTexture);
				m_material.GetCpu().useAlbedoMap = true;
			}
			
			if (!meshData.emissiveTextureFilename.empty()) {
				std::cout << meshData.emissiveTextureFilename << std::endl;
				D3D11Utils::CreateTexture(device, context, meshData.emissiveTextureFilename, true, newMesh.emissiveTexture);
				m_material.GetCpu().useEmissiveMap = true;
			}
			
			if (!meshData.heightTextureFilename.empty()) {
				std::cout << meshData.heightTextureFilename << std::endl;
				D3D11Utils::CreateTexture(device, context, meshData.heightTextureFilename, false, newMesh.heightTexture);
				m_constant.GetCpu().useHeightMap = true;
			}
			
			if (!meshData.normalTextureFilename.empty()) {
				std::cout << meshData.normalTextureFilename << std::endl;
				D3D11Utils::CreateTexture(device, context, meshData.normalTextureFilename, false, newMesh.normalTexture);
				m_material.GetCpu().useNormalMap = true;
				// GLTF는 Y를 뒤집어줘야함
				m_material.GetCpu().invertNormalMapY = isGLTF;
			}
			
			if (!meshData.aoTextureFilename.empty()) {
				std::cout << meshData.aoTextureFilename << std::endl;
				D3D11Utils::CreateTexture(device, context, meshData.aoTextureFilename, false, newMesh.aoTexture);
				m_material.GetCpu().useAOMap = true;
			}

			// GLTF 방식으로 metallic과 roughness를 한 Texture에 넣은 (MetalRoughness Texture)
			if (!meshData.metallicTextureFilename.empty() ||
				!meshData.roughnessTextureFilename.empty()) {
				std::cout << meshData.metallicTextureFilename << std::endl;
				std::cout << meshData.roughnessTextureFilename << std::endl;

				D3D11Utils::CreateMetallicRoughnessTexture(
					device, context, 
					meshData.metallicTextureFilename, 
					meshData.roughnessTextureFilename, 
					newMesh.metallicRoughnessTexture);
			}

			if (!meshData.metallicTextureFilename.empty())
				m_material.GetCpu().useMetallicMap = true;

			if (!meshData.roughnessTextureFilename.empty())
				m_material.GetCpu().useRoughnessMap = true;

			// 모델의 모든 Mesh가 같은 Buffer를 사용
			newMesh.meshConstGPU = m_constant.Get();
			newMesh.basicMaterialConstGPU = m_basicMaterial.Get();
			newMesh.materialConstGPU = m_material.Get();

			m_meshes.emplace_back(newMesh);
		}

		m_basicMaterial.GetCpu().hashID = static_cast<Actor*>(GetOwner())->GetHashID();
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
			m_material.Upload(context);
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
			ID3D11Buffer* constBuffers[3] = {
				mesh.basicMaterialConstGPU.Get(),
				mesh.meshConstGPU.Get(),
				mesh.materialConstGPU.Get(),
			};
			context->VSSetConstantBuffers(1, 3, constBuffers);
			ID3D11ShaderResourceView* heightResView[1] = { mesh.heightTexture.GetSRV() };
			context->VSSetShaderResources(0, 1, heightResView);

			std::vector<ID3D11ShaderResourceView*> resViews = { 
				mesh.albedoTexture.GetSRV(),
				mesh.normalTexture.GetSRV(),
				mesh.aoTexture.GetSRV(),
				mesh.metallicRoughnessTexture.GetSRV(),
				mesh.emissiveTexture.GetSRV()
			};
			context->PSSetShaderResources(0, UINT(resViews.size()), resViews.data());
			context->PSSetConstantBuffers(1, 3, constBuffers);

			context->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &mesh.stride, &mesh.offset);
			context->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
			context->DrawIndexed(mesh.indexCount, 0, 0);
		}
	}

	void ModelComponent::RenderNormal(ComPtr<ID3D11DeviceContext>& context)
	{
		if (!m_drawNormal)
			return;
		// Normal Vector 그리기
		for (const auto& mesh : m_meshes) {
			ID3D11Buffer* constBuffers[3] = {mesh.basicMaterialConstGPU.Get(),
											 mesh.meshConstGPU.Get(),
											 mesh.materialConstGPU.Get()};
			context->GSSetConstantBuffers(1, 3, constBuffers);
			context->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &mesh.stride, &mesh.offset);
			context->Draw(mesh.vertexCount, 0);
		}
	}

	void ModelComponent::RenderPoints(ComPtr<ID3D11DeviceContext>& context)
	{
		for (const auto& mesh : m_meshes) {
			ID3D11Buffer* constBuffers[3] = {
				mesh.basicMaterialConstGPU.Get(),
				mesh.meshConstGPU.Get(),
				mesh.materialConstGPU.Get()
			};
			context->VSSetConstantBuffers(1, 3, constBuffers);

			context->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &mesh.stride, &mesh.offset);
			context->Draw(mesh.indexCount, 0);
		}
	}

}