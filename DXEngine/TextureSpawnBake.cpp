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

	void TextureSpawnBake::LoadBakedData(const std::string& path, StructuredBuffer<Vector3>& outBuffer, UINT& outCount)
	{
		std::ifstream fin(m_presetPath + path, std::ios::binary);
		if (!fin.is_open()) {
			outCount = 0;
			return;
		}

		// 개수 읽기
		fin.read(reinterpret_cast<char*>(&outCount), sizeof(UINT));

		if (outCount > 0) {
			std::vector<Vector3> positions(outCount);
			fin.read(reinterpret_cast<char*>(positions.data()), sizeof(Vector3) * outCount);

			// 버퍼 생성 및 데이터 전송
			auto device = GET_SINGLE(RenderBase)->GetDevice();
			auto context = GET_SINGLE(RenderBase)->GetContext();

			outBuffer.Initialize(device.Get(), outCount);
			outBuffer.SetData(positions);
			outBuffer.Upload(context.Get());
		}

		fin.close();
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

		// SRV 해제 (기존 코드)
		ID3D11ShaderResourceView* nullSRV[3] = { nullptr, nullptr, nullptr };
		context->CSSetShaderResources(0, 3, nullSRV);

		// Sampler 해제 (기존 코드)
		ID3D11SamplerState* nullSampler = nullptr;
		context->CSSetSamplers(0, 1, &nullSampler);

		// [!!! 필수 추가 !!!] UAV 바인딩 해제
		// 해제하지 않으면 이후 Download()에서 GPU->Staging 복사가 실패합니다.
		ID3D11UnorderedAccessView* nullUAV = nullptr;
		UINT cleanCount = 0;
		// AppendBuffer가 u0 슬롯을 사용하므로 0번 슬롯 해제
		context->CSSetUnorderedAccessViews(0, 1, &nullUAV, &cleanCount);
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

		// 실제 데이터보다 validCount가 클 수 없도록 클램핑 (AppendBuffer 크기 초과 방지)
		if (m_validCount > m_maxPoints) m_validCount = m_maxPoints;

		std::ofstream fout(m_presetPath + outputPath, std::ios::binary);

		if (!fout.is_open())
		{
			std::cout << "Failed to open file: " << m_presetPath + outputPath << std::endl;
			return;
		}
		// 개수 먼저 저장
		fout.write(reinterpret_cast<const char*>(&m_validCount), sizeof(UINT));

		// 유효한 개수만큼 데이터 저장
		for (UINT i = 0; i < m_validCount; ++i)
		{
			Vector3 pos = m_outputBuffer.Get(i);
			fout.write(reinterpret_cast<const char*>(&pos), sizeof(Vector3));
		}
		fout.close();
	}
}