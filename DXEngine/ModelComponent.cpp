#include "pch.h"
#include "ModelComponent.h"
#include "Actor.h"
#include "RenderBase.h"

namespace DE {

	void ModelComponent::Initialize() {
		Super::Initialize();
		// BasicMaterial 버퍼 초기화
		m_basicMaterial.Initialize();
	}

	void ModelComponent::Update(const float& deltaTime) {
		Super::Update(deltaTime);

		// HashID 업데이트 (Picking 등을 위해 필요)
		if (GetOwner()) {
			m_basicMaterial.GetCpu().hashID = static_cast<Actor*>(GetOwner())->GetHashID();
		}
		m_basicMaterial.Upload();
	}

	void ModelComponent::SetModel(const std::string& name, const std::string& basePath, bool isGLTF)
	{
		// ModelManager에 로드 위임
		m_modelIndex = ModelManager::Get().LoadModel(name, basePath, isGLTF);
	}

	void ModelComponent::SetModel(const std::string& name, const MeshData& meshData)
	{
		// ModelManager에 로드 위임
		m_modelIndex = ModelManager::Get().LoadModel(name, meshData);
	}

	const void ModelComponent::SetBasicMaterial(const BasicMaterialConstants& consts)
	{
		m_basicMaterial.GetCpu() = consts;
		m_basicMaterial.Upload();
	}

	void ModelComponent::Render() {
		Super::Render();

		if (m_modelIndex < 0) return;

		// 1. 모델 데이터 가져오기
		Model* model = ModelManager::Get().GetModel(m_modelIndex);
		if (!model) return;

		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		// 2. BasicMaterial (HashID) 바인딩 - Slot 2
		// VS, PS 모두에 바인딩 (기존 로직 유지)
		context->VSSetConstantBuffers(2, 1, m_basicMaterial.GetAddressOf());
		context->PSSetConstantBuffers(2, 1, m_basicMaterial.GetAddressOf());

		// 3. 모델의 각 메쉬에 대해 렌더링
		for (size_t i = 0; i < model->meshes.size(); ++i) {
			const auto& mesh = model->meshes[i];
			int materialIndex = model->materialIndices[i];

			// 4. MaterialSystem을 통해 재질 바인딩 (Texture, 상수버퍼 등) - Slot 3
			MaterialSystem::Get().BindMaterial(materialIndex);

			// 5. Mesh 버퍼 바인딩 및 그리기
			context->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &mesh.stride, &mesh.offset);
			context->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
			context->DrawIndexed(mesh.indexCount, 0, 0);
		}
	}

	void ModelComponent::RenderNormal()
	{
		if (!m_drawNormal || m_modelIndex < 0) return;

		Model* model = ModelManager::Get().GetModel(m_modelIndex);
		if (!model) return;

		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		// GS를 사용하는 Normal Vector 렌더링
		for (size_t i = 0; i < model->meshes.size(); ++i) {
			const auto& mesh = model->meshes[i];

			// BasicMaterial 바인딩 (GS)
			context->GSSetConstantBuffers(2, 1, m_basicMaterial.GetAddressOf());

			// Material 상수 버퍼 가져와서 바인딩 (GS Slot 3)
			// MaterialSystem::BindMaterial은 PS/VS만 처리하므로 직접 가져옴
			int materialIdx = model->materialIndices[i];
			auto& matBuffer = MaterialSystem::Get().GetMaterialConstBuffer(materialIdx);
			context->GSSetConstantBuffers(3, 1, matBuffer.GetAddressOf());

			context->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &mesh.stride, &mesh.offset);
			context->Draw(mesh.vertexCount, 0);
		}
	}

	void ModelComponent::RenderPoints()
	{
		if (m_modelIndex < 0) return;

		Model* model = ModelManager::Get().GetModel(m_modelIndex);
		if (!model) return;

		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		for (size_t i = 0; i < model->meshes.size(); ++i) {
			const auto& mesh = model->meshes[i];

			// BasicMaterial 바인딩
			context->VSSetConstantBuffers(2, 1, m_basicMaterial.GetAddressOf());

			// Material 바인딩
			int materialIdx = model->materialIndices[i];
			auto& matBuffer = MaterialSystem::Get().GetMaterialConstBuffer(materialIdx);
			context->VSSetConstantBuffers(3, 1, matBuffer.GetAddressOf());

			context->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &mesh.stride, &mesh.offset);
			context->Draw(mesh.indexCount, 0);
		}
	}
}