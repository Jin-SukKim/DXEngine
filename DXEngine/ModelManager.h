#pragma once
#include "Mesh2.h"
#include "MeshData.h"

namespace DE {

struct Model {
	std::vector<Mesh2> meshes;
	std::string name;
	std::vector<int> materialIndices; // 각 Mesh에 대응되는 Material Index
};

struct MeshRange {
	UINT indexCount;
	UINT startIndexLocation;
	UINT baseVertexLocation;
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

	Model* GetModel(int index);
	MeshRange GetMeshRange(int index) { return m_meshRanges[index]; }
	void BindBuffersForRender();
private:
	void Load(std::vector<Mesh2>& outMeshes, std::vector<int>& outMaterialIndices, const std::string& fullpath, const std::string& basePath, const std::string& filename, bool isGLTF = false);
	void Load(std::vector<Mesh2>& outMeshes, std::vector<int>& outMaterialIndices, const std::string& fullpath, const MeshData& mesh, bool isGLTF = false);
	void Load(std::vector<Mesh2>& outMeshes, std::vector<int>& outMaterialIndices, const std::string& fullpath, const std::vector<MeshData>& meshes, bool isGLTF = false);
	void UpdateToGlobalBuffer(const std::vector<Vertex>& newVertices, const std::vector<UINT>& newIndices);
private:
	std::unordered_map<std::string, int> m_pathToIdx;
	std::vector<std::unique_ptr<Model>> m_allModels;
	std::string presetPath = "..\\Assets\\Models\\";

	ComPtr<ID3D11Buffer> m_vertexBuffer;
	ComPtr<ID3D11Buffer> m_indexBuffer;

	UINT m_currentVertexOffset = 0;
	UINT m_currentIndexOffset = 0;

	const UINT MAX_VERTICES = 5000000;
	const UINT MAX_INDICES = 10000000;

	// 이미 로드된 모델의 위치 캐싱
	std::unordered_map<int, MeshRange> m_meshRanges;
};

}

