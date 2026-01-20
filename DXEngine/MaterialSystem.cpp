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

		if (!meshData.metallicTextureFilename.empty() && (meshData.metallicTextureFilename == meshData.roughnessTextureFilename)) {
			std::cout << meshData.metallicTextureFilename << std::endl;
			std::cout << meshData.roughnessTextureFilename << std::endl;
			auto [metallic, roughness] = TextureManager::Get().LoadMetallicRoughnessTexture(meshData.metallicTextureFilename);
			mat.metallicTexture = metallic;
			mat.roughnessTexture = roughness;
		}
		else {
			if (!meshData.metallicTextureFilename.empty()) {
				std::cout << meshData.metallicTextureFilename << std::endl;
				mat.metallicTexture = TextureManager::Get().LoadTexture(meshData.metallicTextureFilename, false);
			}

			if (!meshData.roughnessTextureFilename.empty()) {
				std::cout << meshData.roughnessTextureFilename << std::endl;
				mat.roughnessTexture = TextureManager::Get().LoadTexture(meshData.roughnessTextureFilename, false);
			}
		}
		if (mat.metallicTexture > -1) constants.GetCpu().useMetallicMap = true;
		if (mat.roughnessTexture > -1) constants.GetCpu().useRoughnessMap = true;
		
		int index = static_cast<int>(m_materials.size());
		m_materials.emplace_back(mat);
		m_materialMap[name] = index;

		constants.Upload();
		m_materialConsts.emplace_back(constants);

		SaveToJson(index);

		return index;
	}

	int MaterialSystem::CreateMaterial(const std::string& name, const MaterialConstants& constants, const std::vector<std::string>& texturePaths)
	{
		if (m_materialMap.find(name) != m_materialMap.end())
			return m_materialMap[name];

		ConstantBuffer<MaterialConstants> consts;
		consts.Initialize();

		consts.SetCpuData(constants);

		Material mat;
		mat.name = name;

		if (texturePaths.size() > 0 && !texturePaths[0].empty()) {
			mat.albedoTexture = TextureManager::Get().LoadTexture(texturePaths[0], true);
			if (mat.albedoTexture >= 0) consts.GetCpu().useAlbedoMap = 1;
		}
		if (texturePaths.size() > 1 && !texturePaths[1].empty()) {
			mat.normalTexture = TextureManager::Get().LoadTexture(texturePaths[1], false);
			if (mat.normalTexture >= 0) consts.GetCpu().useNormalMap = 1;
		}
		if (texturePaths.size() > 2 && !texturePaths[2].empty()) {
			mat.metallicTexture = TextureManager::Get().LoadTexture(texturePaths[2], false);
			if (mat.metallicTexture >= 0) consts.GetCpu().useMetallicMap = 1;
		}
		if (texturePaths.size() > 3 && !texturePaths[3].empty()) {
			mat.roughnessTexture = TextureManager::Get().LoadTexture(texturePaths[3], false);
			if (mat.roughnessTexture >= 0) consts.GetCpu().useRoughnessMap = 1;
		}
		if (texturePaths.size() > 4 && !texturePaths[4].empty()) {
			mat.aoTexture = TextureManager::Get().LoadTexture(texturePaths[4], false);
			if (mat.aoTexture >= 0) consts.GetCpu().useAOMap = 1;
		}
		if (texturePaths.size() > 5 && !texturePaths[5].empty()) {
			mat.emissiveTexture = TextureManager::Get().LoadTexture(texturePaths[5], true);
			if (mat.emissiveTexture >= 0) consts.GetCpu().useEmissiveMap = 1;
		}
		if (texturePaths.size() > 6 && !texturePaths[6].empty()) {
			mat.heightTexture = TextureManager::Get().LoadTexture(texturePaths[6], true);
			if (mat.heightTexture >= 0) consts.GetCpu().useHeightMap = 1;
		}

		int index = static_cast<int>(m_materials.size());
		m_materials.push_back(mat);
		m_materialMap[name] = index;

		consts.Upload();
		m_materialConsts.emplace_back(consts);

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
		ID3D11ShaderResourceView* views[6] = { nullptr, };

		views[0] = TextureManager::Get().GetTextureSRV(mat.albedoTexture);
		views[1] = TextureManager::Get().GetTextureSRV(mat.normalTexture);
		views[2] = TextureManager::Get().GetTextureSRV(mat.aoTexture);
		views[3] = TextureManager::Get().GetTextureSRV(mat.metallicTexture);
		views[4] = TextureManager::Get().GetTextureSRV(mat.roughnessTexture);
		views[5] = TextureManager::Get().GetTextureSRV(mat.emissiveTexture);

		// 슬롯 0번부터 5개 바인딩 (쉐이더 코드와 일치시켜야 함 t0 ~ t4)
		context->PSSetShaderResources(0, 6, views);
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
		int newTexture = TextureManager::Get().LoadTexture(texPath, isGLTF);

		// 로드 실패 시 처리 (옵션: -1이면 텍스처 제거로 처리할 수도 있음)
		if (newTexture < 0) {
			// 텍스처 제거를 원할 경우 아래 플래그를 0으로 설정하는 로직 추가 가능
			return;
		}

		// 2. 슬롯에 따라 인덱스 및 플래그 갱신
		switch (slot)
		{
		case TexSlot::Albedo:
			mat.albedoTexture = newTexture;
			constants.useAlbedoMap = 1; // 텍스처 사용 켜기
			break;
		case TexSlot::Normal:
			mat.normalTexture = newTexture;
			constants.useNormalMap = 1;
			break;
		case TexSlot::Metallic:
			mat.metallicTexture = newTexture;
			constants.useMetallicMap = 1;
			break;
		case TexSlot::Roughness:
			mat.roughnessTexture = newTexture;
			constants.useRoughnessMap = 1;
			break;
		case TexSlot::AO:
			mat.aoTexture = newTexture;
			constants.useAOMap = 1;
			break;
		case TexSlot::Emissive:
			mat.emissiveTexture = newTexture;
			constants.useEmissiveMap = 1;
			break;
		case TexSlot::Height:
			mat.heightTexture = newTexture;
			constants.useHeightMap = 1;
			break;
		}
	}

	const Material* MaterialSystem::GetMaterialData(int materialIdx)
	{
		if (materialIdx < 0 || materialIdx >= m_materials.size())
			return nullptr;
		return &m_materials[materialIdx];
	}

	MaterialConstants& MaterialSystem::GetMaterialConst(int materialIdx)
	{
		return m_materialConsts[materialIdx].GetCpu();
	}

	ConstantBuffer<MaterialConstants>& MaterialSystem::GetMaterialConstBuffer(int materialIdx)
	{
		return m_materialConsts[materialIdx];
	}

	void MaterialSystem::SaveToJson(int materialIdx)
	{
		const Material* mat = GetMaterialData(materialIdx);
		if (!mat) return;

		nlohmann::ordered_json data;
		data["Name"] = mat->name;

		// Constants 저장
		MaterialConstants& constants = m_materialConsts[materialIdx].GetCpu();
		data["Albedo"] = { constants.albedoFactor.x, constants.albedoFactor.y, constants.albedoFactor.z };
		data["Roughness"] = constants.roughnessFactor;
		data["Metallic"] = constants.metallicFactor;
		data["Emission"] = { constants.emissionFactor.x, constants.emissionFactor.y, constants.emissionFactor.z };

		// Textures 저장 
		if (mat->albedoTexture >= 0)
			data["Textures"]["albedo"] = TextureManager::Get().GetTexturePath(mat->albedoTexture);
		if (mat->normalTexture >= 0) 
			data["Textures"]["normal"] = TextureManager::Get().GetTexturePath(mat->normalTexture);
		if (mat->metallicTexture >= 0) 
			data["Textures"]["metallic"] = TextureManager::Get().GetTexturePath(mat->metallicTexture);
		if (mat->roughnessTexture >= 0) 
			data["Textures"]["roughness"] = TextureManager::Get().GetTexturePath(mat->roughnessTexture);
		if (mat->aoTexture >= 0) 
			data["Textures"]["ao"] = TextureManager::Get().GetTexturePath(mat->aoTexture);
		if (mat->emissiveTexture >= 0) 
			data["Textures"]["emissive"] = TextureManager::Get().GetTexturePath(mat->emissiveTexture);
		if (mat->heightTexture >= 0) 
			data["Textures"]["height"] = TextureManager::Get().GetTexturePath(mat->heightTexture);

		std::filesystem::path baseDir = "../Assets/Materials/";
		std::filesystem::path materialName = std::filesystem::path(mat->name).stem(); // 확장자 제거된 이름

		// / 연산자가 자동으로 경로 구분자를 관리해줍니다.
		std::filesystem::path fullPath = baseDir / materialName.replace_extension(".json");

		std::ofstream file(fullPath);

		if (file.is_open()) {
			file << data.dump(4);
			file.close();

			std::cout << mat->name + "Material Saved." << std::endl;
		}
		else {
			std::cout << "Cannot open file for Saving Material." + fullPath.string() << std::endl;
		}
	}
}