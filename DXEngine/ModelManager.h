#pragma once
#include "Mesh2.h"
#include "MeshData.h"
#include "StructuredBuffer.h"
namespace DE {

struct Model {
	std::vector<Mesh2> meshes;
	std::string name;
	std::vector<int> materialIndices; // 각 Mesh에 대응되는 Material Index
};

class ModelManager
{
public:

	static ModelManager& Get() {
		static ModelManager instance;
		return instance;
	}

	void Initialize();
	int LoadModel(const std::string& name, const std::string& basePath = "", bool isGLTF = false);
	int LoadModel(const std::string& name, const MeshData& meshData);

	void BindModelMesh(int index);

	Model* GetModel(int index);

private:
	void Load(std::vector<Mesh2>& outMeshes, std::vector<int>& outMaterialIndices, const std::string& fullpath, const std::string& basePath, const std::string& filename, bool isGLTF = false);
	void Load(std::vector<Mesh2>& outMeshes, std::vector<int>& outMaterialIndices, const std::string& fullpath, const MeshData& mesh, bool isGLTF = false);
	void Load(std::vector<Mesh2>& outMeshes, std::vector<int>& outMaterialIndices, const std::string& fullpath, const std::vector<MeshData>& meshes, bool isGLTF = false);
private:
	std::unordered_map<std::string, int> m_pathToIdx;
	std::vector<std::unique_ptr<Model>> m_allModels;
	std::string presetPath = "..\\Assets\\Models\\";

	StructuredBuffer<Vector3> m_meshVertex;
	StructuredBuffer<uint32_t> m_meshIndices;
};

}

