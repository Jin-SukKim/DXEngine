#pragma once

struct aiNode;
struct aiScene;
struct aiMesh;
struct aiMaterial;
enum aiTextureType;

namespace DE {
	struct MeshData;

	class ModelLoader
	{
	public:
		void Load(const std::string& basePath, const std::string& filename, bool revertNormals, bool calculateNormals = false);
		
		void ProcessNode(aiNode* node, const aiScene* scene, Matrix tr);
		// Node에 저장된 Mesh 데이터를 읽어와서 저장
		MeshData ProcessMesh(aiMesh* mesh, const aiScene* scene);
		
		std::string ReadTextureFilename(const aiScene* scene, aiMaterial* material, aiTextureType type);

		// Normal Mapping을 하기 위핸 Tangent Vector
		void UpdateTangents();

		std::vector<MeshData>& GetMeshes() { return m_meshes; }
	private:
		// lower case로 변환한 file의 extension을 반환
		std::string getExtension(const std::string& filename);
		void updateNormals();
	private:
		std::string m_basePath;
		// 복잡한 모델은 1개의 Mesh가 아닌 여러개의 Mesh로 이루어진 경우가 대부분
		std::vector<MeshData> m_meshes;
		bool m_isGLTF = false; // gltf or fbx
		bool m_revertNormals = false;
	};
}