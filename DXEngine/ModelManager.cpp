#include "pch.h"
#include "ModelManager.h"
#include "GeometryGenerator.h"

namespace DE {
	int ModelManager::LoadModel(const std::string& name, bool isGLTF)
	{
		std::string fullpath = presetPath + name;
		auto it = m_pathToIdx.find(fullpath);

		// 이미 Load되어 있는 Model
		if (it != m_pathToIdx.end())
			return it->second;

		auto newModel = std::make_unique<Model>();
		newModel->name = fullpath;

		Load(newModel->meshes, presetPath, name, isGLTF);

		int index = static_cast<int>(m_allModels.size());
		m_allModels.emplace_back(std::move(newModel));
		m_pathToIdx[fullpath] = index;

		return index;
	}

	int ModelManager::LoadModel(const std::string& name, const MeshData& meshData)
	{
		std::string fullpath = presetPath + name;
		auto it = m_pathToIdx.find(fullpath);

		// 이미 Load되어 있는 Model
		if (it != m_pathToIdx.end())
			return it->second;

		auto newModel = std::make_unique<Model>();
		newModel->name = fullpath;

		Load(newModel->meshes, meshData, false);

		int index = static_cast<int>(m_allModels.size());
		m_allModels.emplace_back(std::move(newModel));
		m_pathToIdx[fullpath] = index;

		return index;
	}

	Model* ModelManager::GetModel(int index)
	{
		// 유효성 검사 (매우 중요)
		if (index < 0 || index >= m_allModels.size())
			return nullptr; // 혹은 Default Missing Model 반환

		return m_allModels[index].get();
	}

	void ModelManager::Load(std::vector<Mesh2>& outMeshes, const std::string& basePath, const std::string& filename, bool isGLTF)
	{
		std::vector<MeshData> meshes = GeometryGenerator::ReadFromFile(basePath, filename);
		Load(outMeshes, meshes, isGLTF);
	}

	void ModelManager::Load(std::vector<Mesh2>& outMeshes, const MeshData& mesh, bool isGLTF)
	{
		Load(outMeshes, std::vector<MeshData>{mesh}, isGLTF);
	}
	
	// TODO: MaterialManager를 만들면 Model을 Load할때 함께 Loading되는 Material을 Manager에 저장
	void ModelManager::Load(std::vector<Mesh2>& outMeshes, const std::vector<MeshData>& meshes, bool isGLTF)
	{
		ComPtr<ID3D11Device>& device = GET_SINGLE(RenderBase)->GetDevice();
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		for (const auto& meshData : meshes) {
			Mesh2 newMesh;
			D3D11Utils::CreateVertexBuffer(device, meshData.vertices, newMesh.vertexBuffer);
			D3D11Utils::CreateIndexBuffer(device, meshData.indices, newMesh.indexBuffer);

			newMesh.indexCount = UINT(meshData.indices.size());
			newMesh.vertexCount = UINT(meshData.vertices.size());
			newMesh.stride = UINT(sizeof(Vertex));

			outMeshes.emplace_back(newMesh);
		}
	}


}