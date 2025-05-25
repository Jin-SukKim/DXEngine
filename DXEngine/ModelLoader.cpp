#include "pch.h"
#include "ModelLoader.h"

// vcpkg install assimp:x64-windows
// Preprocessor definitions에 NOMINMAX 추가
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <filesystem>
#include <fstream>
#include <DirectXMesh.h>

#include "MeshData.h"

namespace DE {

	void ModelLoader::Load(const std::string& basePath, const std::string& filename, bool revertNormals, bool calculateNormals)
	{
		// References
		// https://github.com/assimp/assimp
		// https://learnopengl.com/Model-Loading/Assimp
		// https://ciel45.tistory.com/110
		// https://cutecatgame.tistory.com/11

		// GLTF 포맷인지 확인
		if (getExtension(filename) == ".gltf") {
			m_isGLTF = true;
			m_revertNormals = revertNormals;
		}

		this->m_basePath = basePath;
		
		// https://assimp-docs.readthedocs.io/en/latest/usage/use_the_lib.html
		Assimp::Importer importer;

		const aiScene* pScene = importer.ReadFile(
			m_basePath + filename,
			aiProcess_Triangulate | aiProcess_ConvertToLeftHanded); // DirectX는 왼손 좌표계

		if (!pScene) {
			std::cout << "Failed to read file: " << m_basePath + filename << std::endl;
			auto errorDescription = importer.GetErrorString();
			std::cout << "Assimp error: " << errorDescription << std::endl;
		}
		else {
			Matrix tr; // Initial Transform
			ProcessNode(pScene->mRootNode, pScene, tr);
		}

		// Vertex Normal이 없는 경우 직접 계산 (참고)
		if (calculateNormals)
			updateNormals();

		UpdateTangents();
	}

	void ModelLoader::ProcessNode(aiNode* node, const aiScene* scene, Matrix tr)
	{
		// https://assimp-docs.readthedocs.io/en/latest/usage/use_the_lib.html#data-structures
		Matrix m;
		ai_real* temp = &node->mTransformation.a1;
		float* mTemp = &m._11;
		// 4x4 Matrix
		for (int t = 0; t < 16; t++) {
			mTemp[t] = float(temp[t]);
		}
		m = m.Transpose() * tr;

		for (UINT i = 0; i < node->mNumMeshes; ++i) {
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			// 저장된 Mesh 데이타 읽기
			MeshData newMesh = ProcessMesh(mesh, scene);

			// Mesh의 초기 Transform으로 position을 변환해 저장
			for (auto& v : newMesh.vertices) 
				v.position = Vector3::Transform(v.position, m);
			
			m_meshes.emplace_back(newMesh);
		}

		// Tree 형태로 데이터가 저장되어 있기 때문에 재귀 호출로 타고 내려가면서 확인
		for (UINT i = 0; i < node->mNumChildren; i++) {
			ProcessNode(node->mChildren[i], scene, m);
		}
	}

	MeshData ModelLoader::ProcessMesh(aiMesh* mesh, const aiScene* scene)
	{
		// 저장할 데이터
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		for (UINT i = 0; i < mesh->mNumVertices; ++i) {
			Vertex vertex;

			// Position 저장
			vertex.position.x = mesh->mVertices[i].x;
			vertex.position.y = mesh->mVertices[i].y;
			vertex.position.z = mesh->mVertices[i].z;

			// Noraml Vector 저장
			vertex.normalModel.x = mesh->mNormals[i].x;
			// GLTF는 오른손 좌표계
			if (m_isGLTF) {
				vertex.normalModel.y = mesh->mNormals[i].z;
				vertex.normalModel.z = -mesh->mNormals[i].y;
			}
			else {
				vertex.normalModel.y = mesh->mNormals[i].y;
				vertex.normalModel.z = mesh->mNormals[i].z;
			}

			// normal vector를 뒤집어야 한다면
			if (m_revertNormals) 
				vertex.normalModel *= -1.f;
			
			vertex.normalModel.Normalize(); // Direction이므로 normalize

			// Texture 좌표 저장
			if (mesh->mTextureCoords[0]) {
				vertex.texcoord.x = (float)mesh->mTextureCoords[0][i].x;
				vertex.texcoord.y = (float)mesh->mTextureCoords[0][i].y;
			}

			vertices.emplace_back(vertex);
		}

		// Index 순서 저장
		for (UINT i = 0; i < mesh->mNumFaces; ++i) {
			aiFace face = mesh->mFaces[i]; // Mesh의 가장 작은 단위 (보통 삼각형이나 사각형등 다른 것도 가능)
			for (UINT j = 0; j < face.mNumIndices; j++)
				indices.emplace_back(face.mIndices[j]);
		}

		MeshData newMesh;
		newMesh.vertices = vertices;
		newMesh.indices = indices;

		// http://assimp.sourceforge.net/lib_html/materials.html
		if (mesh->mMaterialIndex >= 0) { // Model의 Material이 있다면
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

			// albedo
			newMesh.albedoTextureFilename = ReadTextureFilename(scene, material, aiTextureType_BASE_COLOR);
			// albedo가 없다면 diffuse로 찾아보기 (PBR에선 albedo와 diffuse가 비슷하게 사용)
			if (newMesh.albedoTextureFilename.empty())
				newMesh.albedoTextureFilename = ReadTextureFilename(scene, material, aiTextureType_DIFFUSE);
			// emissive
			newMesh.emissiveTextureFilename = ReadTextureFilename(scene, material, aiTextureType_EMISSIVE);
			// height map
			newMesh.heightTextureFilename = ReadTextureFilename(scene, material, aiTextureType_HEIGHT);
			// Normal Map
			newMesh.normalTextureFilename = ReadTextureFilename(scene, material, aiTextureType_NORMALS);
			// Metallic
			newMesh.metallicTextureFilename = ReadTextureFilename(scene, material, aiTextureType_METALNESS);
			// Roughness
			newMesh.roughnessTextureFilename = ReadTextureFilename(scene, material, aiTextureType_DIFFUSE_ROUGHNESS);
			// AO map
			newMesh.aoTextureFilename = ReadTextureFilename(scene, material, aiTextureType_AMBIENT_OCCLUSION);
			// AO가 없다면
			if (newMesh.aoTextureFilename.empty()) 
				newMesh.aoTextureFilename = ReadTextureFilename(scene, material, aiTextureType_LIGHTMAP);
			// Opacity(불투명도) map
			newMesh.opacityTextureFilename = ReadTextureFilename(scene, material, aiTextureType_OPACITY);
			if (!newMesh.opacityTextureFilename.empty()) {
				std::cout << newMesh.albedoTextureFilename << std::endl;
				std::cout << "Opacity " << newMesh.opacityTextureFilename << std::endl;
			}

			// 디버깅용
			// for (size_t i = 0; i < 22; i++) {
			//    cout << i << " " << ReadTextureFilename(material,
			//    aiTextureType(i))
			//         << endl;
			//}
		}
		return newMesh;
	}

	std::string ModelLoader::ReadTextureFilename(const aiScene* scene, aiMaterial* material, aiTextureType type)
	{
		// Texture 파일이 있다면
		if (material->GetTextureCount(type) > 0) {
			aiString filePath;
			// Texture 파일 이름 가져오기
			material->GetTexture(type, 0, &filePath);

			std::string fullPath = m_basePath + std::string(std::filesystem::path(filePath.C_Str()).filename().string());

			// 실제로 파일이 존재하는지 확인
			if (!std::filesystem::exists(fullPath)) {
				// 파일이 없을 경우 혹시 fbx 자체에 Embedded인지 확인 (fbx 파일 안에 저장되어 있는지)
				const aiTexture* texture = scene->GetEmbeddedTexture(filePath.C_Str());
				if (texture) {
					// Embedded Texture가 존재하고 png일 경우 저장
					if (std::string(texture->achFormatHint).find("png") != std::string::npos) {
						std::ofstream fs(fullPath.c_str(), std::ios::binary | std::ios::out);
						// 참고: compressed format일 경우 texture->mHeight가 0
						fs.write((char*)texture->pcData, texture->mWidth);
						fs.close();
					}
				}
				else {
					std::cout << fullPath << " doesn'y exists. Return empty filename." << std::endl;
					return "";
				}
			}

			return fullPath;
		}
		
		return "";
	}

	void ModelLoader::UpdateTangents()
	{
		// https://github.com/microsoft/DirectXMesh/wiki/ComputeTangentFrame
		for (auto& m : m_meshes) {
			std::vector<DirectX::XMFLOAT3> positions(m.vertices.size());
			std::vector<DirectX::XMFLOAT3> normals(m.vertices.size());
			std::vector<DirectX::XMFLOAT2> texcoords(m.vertices.size());
			std::vector<DirectX::XMFLOAT3> tangents(m.vertices.size());
			std::vector<DirectX::XMFLOAT3> bitangents(m.vertices.size());

			for (size_t i = 0; i < m.vertices.size(); ++i) {
				auto& v = m.vertices[i];
				positions[i] = v.position;
				normals[i] = v.normalModel;
				texcoords[i] = v.texcoord;
			}

			// DirectX의 Tangent, bitangent 등을 계산해주는 함수
			DirectX::ComputeTangentFrame(
				m.indices.data(), m.indices.size() / 3,
				positions.data(), normals.data(), texcoords.data(),
				m.vertices.size(), tangents.data(),
				bitangents.data());

			for (size_t i = 0; i < m.vertices.size(); ++i)
				m.vertices[i].tangentModel = tangents[i];
			
		}

	}

	std::string ModelLoader::getExtension(const std::string& filename)
	{
		// filename으로부터 extension을 가져오기
		std::string ext(std::filesystem::path(filename).extension().string());
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower); // loop와 동일하게 모든 원소에 지정한 변환을 해줌
		return ext; 
	}

	void ModelLoader::updateNormals()
	{
		// 노멀 벡터가 없는 경우를 대비하여 다시 계산
		// 한 위치에는 한 버텍스만 있어야 연결 관계를 찾을 수 있음

		// DirectXMesh의 ComputeNormals()과 비슷합니다.
		// https://github.com/microsoft/DirectXMesh/wiki/ComputeNormals
		for (auto& m : m_meshes) {
			std::vector<Vector3> normalsTemp(m.vertices.size(), Vector3(0.f));
			std::vector<float> weightsTemp(m.vertices.size(), 0.f);

			for (int i = 0; i < m.indices.size(); i += 3) {
				int idx0 = m.indices[i];
				int idx1 = m.indices[i + 1];
				int idx2 = m.indices[i + 2];

				auto v0 = m.vertices[idx0];
				auto v1 = m.vertices[idx1];
				auto v2 = m.vertices[idx2];

				auto faceNormal = (v1.position - v0.position).Cross(v2.position - v0.position);

				normalsTemp[idx0] += faceNormal;
				normalsTemp[idx1] += faceNormal;
				normalsTemp[idx2] += faceNormal;
				
				weightsTemp[idx0] += 1.f;
				weightsTemp[idx1] += 1.f;
				weightsTemp[idx2] += 1.f;
			}

			for (int i = 0; i < m.vertices.size(); i++) {
				if (weightsTemp[i] > 0.f) {
					m.vertices[i].normalModel = normalsTemp[i] / weightsTemp[i];
					m.vertices[i].normalModel.Normalize();
				}
			}
		}
	}
}