#include "pch.h"
#include "ModelManager.h"
#include "GeometryGenerator.h"
#include "MaterialSystem.h"

namespace DE {
	void ModelManager::Initialize()
	{
		auto& device = GET_SINGLE(RenderBase)->GetDevice();
		D3D11Utils::CreateVertexBufferPool<Vertex>(device, MAX_VERTICES, m_vertexBuffer);
		D3D11Utils::CreateIndexBufferPool(device, MAX_INDICES, m_indexBuffer);

		MeshData quadMesh = GeometryGenerator::MakeSquare(1.0f);
		LoadModel("QuadModel", quadMesh);
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

		UINT startIdx = m_currentIndexOffset;
		UINT startVert = m_currentVertexOffset;

		// 실제 로드는 presetPath 추가
		std::string fullpath = presetPath + relativePath;
		Load(newModel->meshes, newModel->materialIndices, relativePath, presetPath + basePath, name, isGLTF);

		int index = static_cast<int>(m_allModels.size());

		UINT totalIndexCount = m_currentIndexOffset - startIdx;
		m_meshRanges[index] = { totalIndexCount, startIdx, startVert };

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

		UINT startIdx = m_currentIndexOffset;
		UINT startVert = m_currentVertexOffset;

		Load(newModel->meshes, newModel->materialIndices, name, meshData, false);

		int index = static_cast<int>(m_allModels.size());

		UINT totalIndexCount = m_currentIndexOffset - startIdx;
		m_meshRanges[index] = { totalIndexCount, startIdx, startVert };

		m_allModels.emplace_back(std::move(newModel));
		m_pathToIdx[name] = index;

		return index;
	}

	Model* ModelManager::GetModel(int index)
	{
		if (index < 0 || index >= m_allModels.size())
			return nullptr;

		return m_allModels[index].get();
	}

	void ModelManager::BindBuffersForRender()
	{
		auto context = GET_SINGLE(RenderBase)->GetContext();
		// 쿼드 메쉬 바인딩
		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
		context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
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

			newMesh.indexCount = UINT(meshData.indices.size());
			newMesh.vertexCount = UINT(meshData.vertices.size());
			newMesh.stride = UINT(sizeof(Vertex));

			UpdateToGlobalBuffer(meshData.vertices, meshData.indices);

			outMeshes.emplace_back(std::move(newMesh));

			// Material 이름: "ModelName_MeshIndex" (예: "Chair.obj_0")
			std::string materialName = modelName + "_" + std::to_string(i);

			int matIdex = MaterialSystem::Get().CreateMaterial(materialName, meshData, isGLTF);

			outMaterialIndices.emplace_back(matIdex);
		}
	}

	void ModelManager::UpdateToGlobalBuffer(
		const std::vector<Vertex>& newVertices,
		const std::vector<UINT>& newIndices)
	{
		auto context = GET_SINGLE(RenderBase)->GetContext();

		// 1. Vertex Buffer 부분 업데이트
		{
			// 바이트 단위 오프셋 계산
			UINT startByte = m_currentVertexOffset * sizeof(Vertex);
			UINT endByte = startByte + (UINT)(newVertices.size() * sizeof(Vertex));

			// D3D11_BOX를 사용하여 "어디부터 어디까지" 쓸지 지정
			D3D11_BOX destBox = {};
			destBox.left = startByte;
			destBox.right = endByte;
			destBox.top = 0; destBox.bottom = 1;
			destBox.front = 0; destBox.back = 1;

			// pDstBox에 박스 정보를 넘겨줌
			context->UpdateSubresource(m_vertexBuffer.Get(), 0, &destBox, newVertices.data(), 0, 0);
		}

		// 2. Index Buffer 부분 업데이트
		{
			UINT startByte = m_currentIndexOffset * sizeof(UINT);
			UINT endByte = startByte + (UINT)(newIndices.size() * sizeof(UINT));

			D3D11_BOX destBox = {};
			destBox.left = startByte;
			destBox.right = endByte;
			destBox.top = 0; destBox.bottom = 1;
			destBox.front = 0; destBox.back = 1;

			context->UpdateSubresource(m_indexBuffer.Get(), 0, &destBox, newIndices.data(), 0, 0);
		}

		// 오프셋 갱신
		m_currentVertexOffset += (UINT)newVertices.size();
		m_currentIndexOffset += (UINT)newIndices.size();
	}
}