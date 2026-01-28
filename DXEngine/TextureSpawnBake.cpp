#include "pch.h"
#include "TextureSpawnBake.h"
#include "ModelManager.h"
#include "MaterialSystem.h"
#include "TextureManager.h"

namespace DE {
	void TextureSpawnBake::Bake(const std::string& name, const std::string& basePath, bool isGLTF, const std::string& materialName, const std::string& textureType, BakeConsts& consts, const std::string& outputPath)
	{
		int modelIdx = ModelManager::Get().LoadModel(name, basePath, isGLTF);
		if (modelIdx < 0)
			return;

		ID3D11Device* device = GET_SINGLE(RenderBase)->GetDevice().Get();
		ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();

		Model* model = ModelManager::Get().GetModel(modelIdx);
		initBuffers(device, context, model->meshes[0].vertexCPU, model->meshes[0].indexCPU, consts);

		int materialIdx = MaterialSystem::Get().CreateMaterialFromJson(materialName);
		if (materialIdx < 1)
			return;

		setTextureSRV(materialIdx, textureType);
		initShader(device);
		dispatch(context);
		downloadResult(device, context);
		saveToBin(context, outputPath);
	}

	void TextureSpawnBake::LoadBakedDataToVector(const std::string& path, std::vector<Vector3>& outData, UINT& outCount)
	{
		std::string fullPath = m_presetPath + path;

		std::cout << "[Debug] Loading from: " << fullPath << std::endl;

		std::ifstream fin(fullPath, std::ios::binary);
		if (!fin.is_open()) {
			std::cout << "[Error] Cannot open file: " << fullPath << std::endl;
			outCount = 0;
			return;
		}

		// ���� ũ�� Ȯ��
		fin.seekg(0, std::ios::end);
		size_t fileSize = fin.tellg();
		fin.seekg(0, std::ios::beg);
		std::cout << "[Debug] File size: " << fileSize << " bytes" << std::endl;

		// 1. ���� �б�
		fin.read(reinterpret_cast<char*>(&outCount), sizeof(UINT));
		std::cout << "[Debug] Read count: " << outCount << std::endl;

		// ��ȿ�� �˻�
		size_t expectedSize = sizeof(UINT) + sizeof(Vector3) * outCount;
		if (fileSize != expectedSize) {
			std::cout << "[Error] File size mismatch! Expected: " << expectedSize
				<< ", Actual: " << fileSize << std::endl;
			outCount = 0;
			fin.close();
			return;
		}

		if (outCount == 0) {
			std::cout << "[Warning] Count is 0, nothing to load" << std::endl;
			fin.close();
			return;
		}

		if (outCount > 1000000) {  // 100�� �� �ʰ��� ������
			std::cout << "[Error] Count is abnormally large: " << outCount << std::endl;
			outCount = 0;
			fin.close();
			return;
		}

		// 2. ������ �б�
		outData.resize(outCount);
		fin.read(reinterpret_cast<char*>(outData.data()), sizeof(Vector3) * outCount);

		// �б� ���� Ȯ��
		if (!fin) {
			std::cout << "[Error] Failed to read data from file" << std::endl;
			outCount = 0;
			fin.close();
			return;
		}

		// ù 5�� ��� (�����)
		for (UINT i = 0; i < std::min(5u, outCount); ++i) {
			std::cout << "[Debug] Loaded point " << i << ":  "
				<< outData[i].x << ", "
				<< outData[i].y << ", "
				<< outData[i].z << std::endl;
		}

		fin.close();
		std::cout << "[Success] Loaded " << outCount << " points to vector" << std::endl;
	}

	void TextureSpawnBake::LoadBakedData(const std::string& path, StructuredBuffer<Vector3>& outBuffer, UINT& outCount)
	{
		std::vector<Vector3> positions;
		LoadBakedDataToVector(path, positions, outCount);

		if (outCount == 0)
			return;

		// GPU ���� ���� �� ���ε�
		auto device = GET_SINGLE(RenderBase)->GetDevice();
		auto context = GET_SINGLE(RenderBase)->GetContext();

		outBuffer.Initialize(device.Get(), outCount);
		outBuffer.SetData(positions);
		outBuffer.Upload(context.Get());
		std::cout << "[Success] Uploaded " << outCount << " points to GPU" << std::endl;
	}

	void TextureSpawnBake::initBuffers(ID3D11Device* device, ID3D11DeviceContext* context, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, BakeConsts& consts)
	{
		consts.indexCount = static_cast<UINT>(indices.size());

		m_meshVertex.Initialize(device, static_cast<UINT>(vertices.size()));
		m_meshIndices.Initialize(device, consts.indexCount);

		m_meshVertex.SetData(vertices);
		m_meshIndices.SetData(indices);

		m_meshVertex.Upload(context);
		m_meshIndices.Upload(context);

		m_outputBuffer.Initialize(device, m_maxPoints);

		m_consts.Initialize();
		m_consts.SetCpuData(consts);
		m_consts.Upload();
	}

	void TextureSpawnBake::setTextureSRV(const int& materialIdx, const std::string& textureType)
	{
		const Material* mat = MaterialSystem::Get().GetMaterialData(materialIdx);
		if (textureType == "albedo") m_srv = TextureManager::Get().GetTextureSRV(mat->albedoTexture);
		else if (textureType == "emissive") m_srv = TextureManager::Get().GetTextureSRV(mat->emissiveTexture);
		else if (textureType == "metallic") m_srv = TextureManager::Get().GetTextureSRV(mat->metallicTexture);
		else if (textureType == "roughness") m_srv = TextureManager::Get().GetTextureSRV(mat->roughnessTexture);
		else if (textureType == "normal") m_srv = TextureManager::Get().GetTextureSRV(mat->normalTexture);
		else if (textureType == "ao") m_srv = TextureManager::Get().GetTextureSRV(mat->aoTexture);
		else m_srv = nullptr;
	}

	void TextureSpawnBake::initShader(ID3D11Device* device)
	{
		m_bakeCS.Initialize(device, L"TextureSpawnBakeCS.hlsl");
	}

	void TextureSpawnBake::dispatch(ID3D11DeviceContext* context)
	{
		context->CSSetSamplers(0, 2, RenderBase::graphicsCommon.sampleStates.data());

		ID3D11ShaderResourceView* srvs[] = {
			m_meshVertex.GetSRV(),
			m_meshIndices.GetSRV(),
			m_srv.Get()
		};
		context->CSSetShaderResources(0, 3, srvs);

		UINT initCounts = 0;
		context->CSSetUnorderedAccessViews(0, 1, m_outputBuffer.GetAddressOfUAV(), &initCounts);
		context->CSSetConstantBuffers(0, 1, m_consts.GetAddressOf());

		UINT triangleCount = m_consts.GetCpu().indexCount / 3;
		UINT groupCount = (triangleCount + 255) / 256;
		m_bakeCS.Dispatch(context, groupCount, 1, 1);

		//// SRV ���� (���� �ڵ�)
		//ID3D11ShaderResourceView* nullSRV[3] = { nullptr, nullptr, nullptr };
		//context->CSSetShaderResources(0, 3, nullSRV);

		//// Sampler ���� (���� �ڵ�)
		//ID3D11SamplerState* nullSampler = nullptr;
		//context->CSSetSamplers(0, 1, &nullSampler);

		//// [!!! �ʼ� �߰� !!!] UAV ���ε� ����
		//// �������� ������ ���� Download()���� GPU->Staging ���簡 �����մϴ�.
		//ID3D11UnorderedAccessView* nullUAV = nullptr;
		//UINT cleanCount = 0;
		//// AppendBuffer�� u0 ������ ����ϹǷ� 0�� ���� ����
		//context->CSSetUnorderedAccessViews(0, 1, &nullUAV, &cleanCount);
	}

	void TextureSpawnBake::downloadResult(ID3D11Device* device, ID3D11DeviceContext* context)
	{
		// ����ü ī��Ʈ(��ȿ ����) ��������
		ComPtr<ID3D11Buffer> countStagingBuffer;
		D3D11_BUFFER_DESC countDesc = {};
		countDesc.ByteWidth = 4;
		countDesc.Usage = D3D11_USAGE_STAGING;
		countDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		ThrowIfFailed(device->CreateBuffer(&countDesc, nullptr, &countStagingBuffer));

		context->CopyStructureCount(countStagingBuffer.Get(), 0, m_outputBuffer.GetUAV());

		D3D11_MAPPED_SUBRESOURCE mappedCount;
		ThrowIfFailed(context->Map(countStagingBuffer.Get(), 0, D3D11_MAP_READ, 0, &mappedCount));
		m_validCount = *reinterpret_cast<UINT*>(mappedCount.pData);
		context->Unmap(countStagingBuffer.Get(), 0);

		if (m_validCount)
			// ������ �ٿ�ε� (StructuredBuffer�� Download �Լ� Ȱ��)
			m_outputBuffer.Download(context);
	}

	void TextureSpawnBake::saveToBin(ID3D11DeviceContext* context, const std::string& outputPath)
	{
		std::string fullPath = m_presetPath + outputPath;

		std::cout << "[Debug] Saving to:  " << fullPath << std::endl;
		std::cout << "[Debug] Valid count: " << m_validCount << std::endl;

		if (m_validCount == 0) {
			std::cout << "[Error] Cannot save:  validCount is 0" << std::endl;
			return;
		}

		// ��ȿ�� ��˻�
		if (m_validCount > m_maxPoints) {
			std::cout << "[Warning] Clamping validCount from " << m_validCount
				<< " to " << m_maxPoints << std::endl;
			m_validCount = m_maxPoints;
		}

		std::ofstream fout(fullPath, std::ios::binary);
		if (!fout.is_open()) {
			std::cout << "[Error] Failed to open file: " << fullPath << std::endl;
			return;
		}

		// 1. ���� ����
		fout.write(reinterpret_cast<const char*>(&m_validCount), sizeof(UINT));
		std::cout << "[Debug] Wrote count: " << m_validCount << std::endl;

		// 2. ������ ����
		for (UINT i = 0; i < m_validCount; ++i) {
			Vector3 pos = m_outputBuffer.Get(i);
			fout.write(reinterpret_cast<const char*>(&pos), sizeof(Vector3));

			// ù 5���� ��� (�����)
			if (i < 5) {
				std::cout << "[Debug] Point " << i << ": "
					<< pos.x << ", " << pos.y << ", " << pos.z << std::endl;
			}
		}

		fout.close();

		// ���� ũ�� Ȯ��
		std::ifstream checkFile(fullPath, std::ios::binary | std::ios::ate);
		size_t fileSize = checkFile.tellg();
		checkFile.close();

		size_t expectedSize = sizeof(UINT) + sizeof(Vector3) * m_validCount;
		std::cout << "[Debug] File size: " << fileSize << " bytes (expected: " << expectedSize << ")" << std::endl;

		if (fileSize == expectedSize) {
			std::cout << "[Success] File saved correctly!" << std::endl;
		}
		else {
			std::cout << "[Error] File size mismatch!" << std::endl;
		}
	}
}