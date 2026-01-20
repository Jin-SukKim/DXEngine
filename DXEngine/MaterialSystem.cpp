#include "pch.h"
#include "MaterialSystem.h"
#include "TextureManager.h"

namespace DE {
	void MaterialSystem::Initialize()
	{
		auto device = GET_SINGLE(RenderBase)->GetDevice();
	}

	int MaterialSystem::CreateMaterial(const std::string& name, const MeshData& meshData, bool isGLTF)
	{
		if (m_materialMap.find(name) != m_materialMap.end())
			return m_materialMap[name];
		
		ConstantBuffer<MaterialConstants> constants;
		constants.Initialize();
		
		Material mat;
		mat.name = name;

		if (!meshData.albedoTextureFilename.empty()) {
			std::cout << meshData.albedoTextureFilename << std::endl;
			mat.albedoTexture = TextureManager::Get().LoadTexture(meshData.albedoTextureFilename, true);
			if (mat.albedoTexture > -1) constants.GetCpu().useAlbedoMap = true;
		}

		if (!meshData.emissiveTextureFilename.empty()) {
			std::cout << meshData.emissiveTextureFilename << std::endl;
			mat.emissiveTexture = TextureManager::Get().LoadTexture(meshData.emissiveTextureFilename, true);
			if (mat.emissiveTexture > -1) constants.GetCpu().useEmissiveMap = true;
		}

		if (!meshData.heightTextureFilename.empty()) {
			std::cout << meshData.heightTextureFilename << std::endl;
			mat.heightTexture = TextureManager::Get().LoadTexture(meshData.heightTextureFilename, false);
			if (mat.heightTexture > -1) constants.GetCpu().useHeightMap = true;
		}

		if (!meshData.normalTextureFilename.empty()) {
			std::cout << meshData.normalTextureFilename << std::endl;
			mat.normalTexture = TextureManager::Get().LoadTexture(meshData.normalTextureFilename, false);
			if (mat.normalTexture > -1) {
				constants.GetCpu().useNormalMap = true;
				// GLTF는 Y를 뒤집어줘야함
				constants.GetCpu().invertNormalMapY = isGLTF;
			}
		}

		if (!meshData.aoTextureFilename.empty()) {
			std::cout << meshData.aoTextureFilename << std::endl;
			mat.aoTexture = TextureManager::Get().LoadTexture(meshData.aoTextureFilename, false);
			if (mat.aoTexture > -1) constants.GetCpu().useAOMap = true;
		}

		// GLTF 방식으로 metallic과 roughness를 한 Texture에 넣은 (MetalRoughness Texture)
		if (!meshData.metallicTextureFilename.empty() ||
			!meshData.roughnessTextureFilename.empty()) {
			std::cout << meshData.metallicTextureFilename << std::endl;
			std::cout << meshData.roughnessTextureFilename << std::endl;

			mat.metallicRoughnessTexture = TextureManager::Get().LoadMetallicRoughnessTexture(meshData.metallicTextureFilename, meshData.roughnessTextureFilename);
		}

		if (!meshData.metallicTextureFilename.empty())
			constants.GetCpu().useMetallicMap = true;

		if (!meshData.roughnessTextureFilename.empty())
			constants.GetCpu().useRoughnessMap = true;

		int index = static_cast<int>(m_materials.size());
		m_materials.push_back(mat);
		m_materialMap[name] = index;

		m_materialConsts.emplace_back(constants);

		return index;
	}

	void MaterialSystem::BindMaterial(int materialIdx)
	{
		auto context = GET_SINGLE(RenderBase)->GetContext();

		if (materialIdx < 0 || materialIdx >= m_materials.size())
			materialIdx = 0; // 유효하지 않으면 기본 재질 사용

		const Material& mat = m_materials[materialIdx];

		// 쉐이더의 지정된 슬롯(예: b2)에 바인딩
		context->PSSetConstantBuffers(3, 1, m_materialConsts[materialIdx].GetAddressOf());
		ID3D11ShaderResourceView* heightResView[1] = { TextureManager::Get().GetTextureSRV(mat.heightTexture) };
		context->VSSetShaderResources(0, 1, heightResView);

		// 2. 텍스처 바인딩
		// TextureManager에서 SRV를 가져와서 바인딩
		ID3D11ShaderResourceView* views[5] = { nullptr, };

		views[0] = TextureManager::Get().GetTextureSRV(mat.albedoTexture);
		views[1] = TextureManager::Get().GetTextureSRV(mat.normalTexture);
		views[2] = TextureManager::Get().GetTextureSRV(mat.aoTexture);
		views[3] = TextureManager::Get().GetTextureSRV(mat.metallicRoughnessTexture);
		views[4] = TextureManager::Get().GetTextureSRV(mat.emissiveTexture);

		// 슬롯 0번부터 5개 바인딩 (쉐이더 코드와 일치시켜야 함 t0 ~ t4)
		context->PSSetShaderResources(0, 5, views);
		context->PSSetConstantBuffers(3, 1, m_materialConsts[materialIdx].GetAddressOf());
	}

	void MaterialSystem::SetTexture(const std::string& matName, TexSlot slot, const std::string& texPath, bool isGLTF)
	{
		auto it = m_materialMap.find(matName);
		if (it == m_materialMap.end())
			return;

		SetTexture(it->second, slot, texPath, isGLTF);
	}

	void MaterialSystem::SetTexture(int matIdx, TexSlot slot, const std::string& texPath, bool isGLTF)
	{
		if (matIdx < 0 || matIdx >= m_materials.size())
			return;

		Material& mat = m_materials[matIdx];
		MaterialConstants& constants = GetMaterialConst(matIdx);

		// 1. TextureManager를 통해 텍스처 로드 (이미 있다면 캐싱된 인덱스 반환)
		int newTexIndex = TextureManager::Get().LoadTexture(texPath, isGLTF);

		// 로드 실패 시 처리 (옵션: -1이면 텍스처 제거로 처리할 수도 있음)
		if (newTexIndex < 0) {
			// 텍스처 제거를 원할 경우 아래 플래그를 0으로 설정하는 로직 추가 가능
			return;
		}

		// 2. 슬롯에 따라 인덱스 및 플래그 갱신
		switch (slot)
		{
		case TexSlot::Albedo:
			mat.albedoTexture = newTexIndex;
			constants.useAlbedoMap = 1; // 텍스처 사용 켜기
			break;
		case TexSlot::Normal:
			mat.normalTexture = newTexIndex;
			constants.useNormalMap = 1;
			break;
		case TexSlot::Metallic:
			mat.metallicRoughnessTexture = newTexIndex;
			constants.useMetallicMap = 1;
			break;
		case TexSlot::Roughness:
			mat.metallicRoughnessTexture = newTexIndex;
			constants.useRoughnessMap = 1;
			break;
		case TexSlot::AO:
			mat.aoTexture = newTexIndex;
			constants.useAOMap = 1;
			break;
		case TexSlot::Emissive:
			mat.emissiveTexture = newTexIndex;
			constants.useEmissiveMap = 1;
			break;
		}
	}

	MaterialConstants& MaterialSystem::GetMaterialConst(int materialIdx)
	{
		return m_materialConsts[materialIdx].GetCpu();
	}

	ConstantBuffer<MaterialConstants>& MaterialSystem::GetMaterialConstBuffer(int materialIdx)
	{
		return m_materialConsts[materialIdx];
	}
}