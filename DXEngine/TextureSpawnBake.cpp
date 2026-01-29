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

	void TextureSpawnBake::LoadBakedData(const std::string& path, StructuredBuffer<Vector3>& outBuffer, UINT& outCount, UINT offset)
	{
		std::string fullPath = m_presetPath + path;

		std::cout << "[Debug] Loading from: " << fullPath << std::endl;

		std::ifstream fin(fullPath, std::ios::binary);
		if (!fin.is_open()) {
			std::cout << "[Error] Cannot open file: " << fullPath << std::endl;
			outCount = 0;
			return;
		}

		// 파일 크기 확인
		fin.seekg(0, std::ios::end);
		size_t fileSize = fin.tellg();
		fin.seekg(0, std::ios::beg);
		std::cout << "[Debug] File size: " << fileSize << " bytes" << std::endl;

		// 1. 개수 읽기
		fin.read(reinterpret_cast<char*>(&outCount), sizeof(UINT));
		std::cout << "[Debug] Read count: " << outCount << std::endl;

		// 유효성 검사
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

		if (outCount > 1000000) {  // 100만 개 초과는 비정상
			std::cout << "[Error] Count is abnormally large: " << outCount << std::endl;
			outCount = 0;
			fin.close();
			return;
		}

		// 2. 데이터 읽기
		std::vector<Vector3> positions(outCount);
		fin.read(reinterpret_cast<char*>(positions.data()), sizeof(Vector3) * outCount);

		// 읽기 성공 확인
		if (!fin) {
			std::cout << "[Error] Failed to read data from file" << std::endl;
			outCount = 0;
			fin.close();
			return;
		}

		// 첫 5개 출력 (디버깅)
		for (UINT i = 0; i < std::min(5u, outCount); ++i) {
			std::cout << "[Debug] Loaded point " << i << ":  "
				<< positions[i].x << ", "
				<< positions[i].y << ", "
				<< positions[i].z << std::endl;
		}

		fin.close();

		// 3. GPU 버퍼 생성 및 업로드
		auto device = GET_SINGLE(RenderBase)->GetDevice();
		auto context = GET_SINGLE(RenderBase)->GetContext();

		for (UINT i = 0; i < positions.size(); ++i)
			outBuffer.Get(offset + i) = positions[i];

		std::cout << "[Success] Loaded " << outCount << " points to GPU" << std::endl;
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
		UINT groupCount = (triangleCount + 1023) / 1024;
		m_bakeCS.Dispatch(context, groupCount, 1, 1);

		//// SRV 해제 (기존 코드)
		//ID3D11ShaderResourceView* nullSRV[3] = { nullptr, nullptr, nullptr };
		//context->CSSetShaderResources(0, 3, nullSRV);

		//// Sampler 해제 (기존 코드)
		//ID3D11SamplerState* nullSampler = nullptr;
		//context->CSSetSamplers(0, 1, &nullSampler);

		//// [!!! 필수 추가 !!!] UAV 바인딩 해제
		//// 해제하지 않으면 이후 Download()에서 GPU->Staging 복사가 실패합니다.
		//ID3D11UnorderedAccessView* nullUAV = nullptr;
		//UINT cleanCount = 0;
		//// AppendBuffer가 u0 슬롯을 사용하므로 0번 슬롯 해제
		//context->CSSetUnorderedAccessViews(0, 1, &nullUAV, &cleanCount);
	}

	void TextureSpawnBake::downloadResult(ID3D11Device* device, ID3D11DeviceContext* context)
	{
		// 구조체 카운트(유효 개수) 가져오기
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
			// 데이터 다운로드 (StructuredBuffer의 Download 함수 활용)
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

		// 유효성 재검사
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

		// 1. 개수 저장
		fout.write(reinterpret_cast<const char*>(&m_validCount), sizeof(UINT));
		std::cout << "[Debug] Wrote count: " << m_validCount << std::endl;

		// 2. 데이터 저장
		for (UINT i = 0; i < m_validCount; ++i) {
			Vector3 pos = m_outputBuffer.Get(i);
			fout.write(reinterpret_cast<const char*>(&pos), sizeof(Vector3));

			// 첫 5개만 출력 (디버깅)
			if (i < 5) {
				std::cout << "[Debug] Point " << i << ": "
					<< pos.x << ", " << pos.y << ", " << pos.z << std::endl;
			}
		}

		fout.close();

		// 파일 크기 확인
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