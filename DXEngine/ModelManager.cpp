#include "pch.h"
#include "ModelManager.h"
#include "GeometryGenerator.h"
#include "MaterialSystem.h"

namespace DE {
	void ModelManager::Initialize()
	{
		ID3D11Device* device = GET_SINGLE(RenderBase)->GetDevice().Get();
		m_meshVertex.InitializeDynamicSRV(device, 1000000);
		m_meshIndices.InitializeDynamicSRV(device, 1000000);
	}

	int ModelManager::LoadModel(const std::string& name, const std::string& basePath, bool isGLTF)
	{
		// 상대 경로 조합 (예: "Models/Chair.obj")
		std::string relativePath = basePath + name;
		auto it = m_pathToIdx.find(relativePath);

		if (it != m_pathToIdx.end())
			return it->second;

		auto newModel = std::make_unique<Model>();
		newModel->name = relativePath;  // 상대 경로 저장

		// 실제 로드는 presetPath 추가
		std::string fullpath = presetPath + relativePath;
		Load(newModel->meshes, newModel->materialIndices, relativePath, presetPath + basePath, name, isGLTF);

		int index = static_cast<int>(m_allModels.size());
		m_allModels.emplace_back(std::move(newModel));
		m_pathToIdx[relativePath] = index;

		return index;
	}

	int ModelManager::LoadModel(const std::string& name, const MeshData& meshData)
	{
		// name은 이미 상대 경로 (예: "ParticleBox")
		auto it = m_pathToIdx.find(name);

		if (it != m_pathToIdx.end())
			return it->second;

		auto newModel = std::make_unique<Model>();
		newModel->name = name;

		Load(newModel->meshes, newModel->materialIndices, name, meshData, false);

		int index = static_cast<int>(m_allModels.size());
		m_allModels.emplace_back(std::move(newModel));
		m_pathToIdx[name] = index;

		return index;
	}

	void ModelManager::BindModelMesh(int index)
	{
		Model* model = GetModel(index);
		if (model) {
			ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();
			m_meshVertex.SetData(model->meshes[0].vertexPosCPU);
			m_meshIndices.SetData(model->meshes[0].indexCPU);
			m_meshVertex.Upload(context);
			m_meshIndices.Upload(context);

			ID3D11ShaderResourceView* srvs[] = { m_meshVertex.GetSRV(), m_meshIndices.GetSRV() };
			context->CSSetShaderResources(2, 2, srvs);
		}
	}

	Model* ModelManager::GetModel(int index)
	{
		if (index < 0 || index >= m_allModels.size())
			return nullptr;

		return m_allModels[index].get();
	}

	void ModelManager::Load(std::vector<Mesh2>& outMeshes, std::vector<int>& outMaterialIndices, const std::string& modelName, const std::string& basePath, const std::string& filename, bool isGLTF)
	{
		std::vector<MeshData> meshes = GeometryGenerator::ReadFromFile(basePath, filename);
		Load(outMeshes, outMaterialIndices, modelName, meshes, isGLTF);
	}

	void ModelManager::Load(std::vector<Mesh2>& outMeshes, std::vector<int>& outMaterialIndices, const std::string& modelName, const MeshData& mesh, bool isGLTF)
	{
		Load(outMeshes, outMaterialIndices, modelName, std::vector<MeshData>{mesh}, isGLTF);
	}
	
	void ModelManager::Load(std::vector<Mesh2>& outMeshes, std::vector<int>& outMaterialIndices, const std::string& modelName, const std::vector<MeshData>& meshes, bool isGLTF)
	{
		ComPtr<ID3D11Device>& device = GET_SINGLE(RenderBase)->GetDevice();
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		for (size_t i = 0; i < meshes.size(); ++i) {
			const auto& meshData = meshes[i];
			Mesh2 newMesh;
			
			D3D11Utils::CreateVertexBuffer(device, meshData.vertices, newMesh.vertexBuffer);
			D3D11Utils::CreateIndexBuffer(device, meshData.indices, newMesh.indexBuffer);

			newMesh.vertexCPU = meshData.vertices;
			newMesh.indexCPU = meshData.indices;

			std::vector<Vector3> vertices;
			for (const auto& vertex : meshData.vertices)
				vertices.push_back(vertex.position);
			newMesh.vertexPosCPU = vertices;

			newMesh.indexCount = UINT(meshData.indices.size());
			newMesh.vertexCount = UINT(meshData.vertices.size());
			newMesh.stride = UINT(sizeof(Vertex));

			outMeshes.emplace_back(std::move(newMesh));

			// Material 이름: "ModelName_MeshIndex" (예: "Chair.obj_0")
			std::string materialName = modelName + "_" + std::to_string(i);

			int matIdex = MaterialSystem::Get().CreateMaterial(materialName, meshData, isGLTF);

			outMaterialIndices.emplace_back(matIdex);
		}
	}
}