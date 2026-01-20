#pragma once
#include "Mesh2.h"
#include "MeshData.h"

namespace DE {

struct Model {
	std::vector<Mesh2> meshes;
	std::string name;
};

class ModelManager
{
public:

	static ModelManager& Get() {
		static ModelManager instance;
		return instance;
	}

	int LoadModel(const std::string& name, const std::string& basePath = "", bool isGLTF = false);
	int LoadModel(const std::string& name, const MeshData& meshData);
	

	Model* GetModel(int index);

private:
	void Load(std::vector<Mesh2>& outMeshes, const std::string& basePath, const std::string& filename, bool isGLTF = false);
	void Load(std::vector<Mesh2>& outMeshes, const MeshData& mesh, bool isGLTF = false);
	void Load(std::vector<Mesh2>& outMeshes, const std::vector<MeshData>& meshes, bool isGLTF = false);
private:
	std::unordered_map<std::string, int> m_pathToIdx;
	std::vector<std::unique_ptr<Model>> m_allModels;
	std::string presetPath = "..\\Assets\\Models\\";
};

}

