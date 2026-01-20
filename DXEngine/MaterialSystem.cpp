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
		// name은 이미 상대 경로 (예: "Models/Chair.obj")
		if (m_materialMap.find(name) != m_materialMap.end())
			return m_materialMap[name];
		
		ConstantBuffer<MaterialConstants> constants;
		constants.Initialize();
		
		Material mat;
		mat.name = name;

		// 텍스처 경로도 상대 경로로 저장됨
		if (!meshData.albedoTextureFilename.empty()) {
			std::cout << "Loading albedo: " << meshData.albedoTextureFilename << std::endl;
			mat.albedoTexture = TextureManager::Get().LoadTexture(meshData.albedoTextureFilename, true);
			if (mat.albedoTexture > -1) constants.GetCpu().useAlbedoMap = true;
		}

		if (!meshData.emissiveTextureFilename.empty()) {
			std::cout << "Loading emissive: " << meshData.emissiveTextureFilename << std::endl;
			mat.emissiveTexture = TextureManager::Get().LoadTexture(meshData.emissiveTextureFilename, true);
			if (mat.emissiveTexture > -1) constants.GetCpu().useEmissiveMap = true;
		}

		if (!meshData.heightTextureFilename.empty()) {
			std::cout << "Loading height: " << meshData.heightTextureFilename << std::endl;
			mat.heightTexture = TextureManager::Get().LoadTexture(meshData.heightTextureFilename, false);
			if (mat.heightTexture > -1) constants.GetCpu().useHeightMap = true;
		}

		if (!meshData.normalTextureFilename.empty()) {
			std::cout << "Loading normal: " << meshData.normalTextureFilename << std::endl;
			mat.normalTexture = TextureManager::Get().LoadTexture(meshData.normalTextureFilename, false);
			if (mat.normalTexture > -1) {
				constants.GetCpu().useNormalMap = true;
				constants.GetCpu().invertNormalMapY = isGLTF;
			}
		}

		if (!meshData.aoTextureFilename.empty()) {
			std::cout << "Loading AO: " << meshData.aoTextureFilename << std::endl;
			mat.aoTexture = TextureManager::Get().LoadTexture(meshData.aoTextureFilename, false);
			if (mat.aoTexture > -1) constants.GetCpu().useAOMap = true;
		}

		if (!meshData.metallicTextureFilename.empty() && (meshData.metallicTextureFilename == meshData.roughnessTextureFilename)) {
			std::cout << "Loading metallic-roughness combined: " << meshData.metallicTextureFilename << std::endl;
			auto [metallic, roughness] = TextureManager::Get().LoadMetallicRoughnessTexture(meshData.metallicTextureFilename);
			mat.metallicTexture = metallic;
			mat.roughnessTexture = roughness;
		}
		else {
			if (!meshData.metallicTextureFilename.empty()) {
				std::cout << "Loading metallic: " << meshData.metallicTextureFilename << std::endl;
				mat.metallicTexture = TextureManager::Get().LoadTexture(meshData.metallicTextureFilename, false);
			}

			if (!meshData.roughnessTextureFilename.empty()) {
				std::cout << "Loading roughness: " << meshData.roughnessTextureFilename << std::endl;
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

		// texturePaths는 이미 상대 경로
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
			mat.heightTexture = TextureManager::Get().LoadTexture(texturePaths[6], false);
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
			materialIdx = 0;

		const Material& mat = m_materials[materialIdx];

		context->PSSetConstantBuffers(3, 1, m_materialConsts[materialIdx].GetAddressOf());
		ID3D11ShaderResourceView* heightResView[1] = { TextureManager::Get().GetTextureSRV(mat.heightTexture) };
		context->VSSetShaderResources(0, 1, heightResView);

		ID3D11ShaderResourceView* views[6] = { nullptr, };

		views[0] = TextureManager::Get().GetTextureSRV(mat.albedoTexture);
		views[1] = TextureManager::Get().GetTextureSRV(mat.normalTexture);
		views[2] = TextureManager::Get().GetTextureSRV(mat.aoTexture);
		views[3] = TextureManager::Get().GetTextureSRV(mat.metallicTexture);
		views[4] = TextureManager::Get().GetTextureSRV(mat.roughnessTexture);
		views[5] = TextureManager::Get().GetTextureSRV(mat.emissiveTexture);

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

		int newTexture = TextureManager::Get().LoadTexture(texPath, isGLTF);

		if (newTexture < 0)
			return;

		switch (slot)
		{
		case TexSlot::Albedo:
			mat.albedoTexture = newTexture;
			constants.useAlbedoMap = 1;
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

	std::string MaterialSystem::SimplifyTexturePath(const std::string& fullPath)
	{
		// "Models/DamagedHelmet/Default_albedo.jpg" → "DamagedHelmet/Default_albedo.jpg"
		// "Models/" 부분 제거
		
		size_t modelsPos = fullPath.find("Models/");
		if (modelsPos != std::string::npos) {
			// "Models/" 이후 부분 반환
			return fullPath.substr(modelsPos + 7); // "Models/" 길이 = 7
		}
		
		// "Models\"로 저장된 경우 (Windows 경로)
		modelsPos = fullPath.find("Models\\");
		if (modelsPos != std::string::npos) {
			return fullPath.substr(modelsPos + 7);
		}
		
		// "Models/"가 없으면 그대로 반환
		return fullPath;
	}

	void MaterialSystem::SaveToJson(int materialIdx)
	{
		const Material* mat = GetMaterialData(materialIdx);
		if (!mat) return;

		nlohmann::ordered_json data;
		data["Name"] = mat->name;

		MaterialConstants& constants = m_materialConsts[materialIdx].GetCpu();
		data["Albedo"] = { constants.albedoFactor.x, constants.albedoFactor.y, constants.albedoFactor.z };
		data["Roughness"] = constants.roughnessFactor;
		data["Metallic"] = constants.metallicFactor;
		data["Emission"] = { constants.emissionFactor.x, constants.emissionFactor.y, constants.emissionFactor.z };

		// 텍스처 경로 저장 시 "Models/" 부분 제거
		if (mat->albedoTexture >= 0) {
			std::string path = TextureManager::Get().GetTexturePath(mat->albedoTexture);
			data["Textures"]["albedo"] = SimplifyTexturePath(path);
		}
		if (mat->normalTexture >= 0) {
			std::string path = TextureManager::Get().GetTexturePath(mat->normalTexture);
			data["Textures"]["normal"] = SimplifyTexturePath(path);
		}
		if (mat->metallicTexture >= 0) {
			std::string path = TextureManager::Get().GetTexturePath(mat->metallicTexture);
			data["Textures"]["metallic"] = SimplifyTexturePath(path);
		}
		if (mat->roughnessTexture >= 0) {
			std::string path = TextureManager::Get().GetTexturePath(mat->roughnessTexture);
			data["Textures"]["roughness"] = SimplifyTexturePath(path);
		}
		if (mat->aoTexture >= 0) {
			std::string path = TextureManager::Get().GetTexturePath(mat->aoTexture);
			data["Textures"]["ao"] = SimplifyTexturePath(path);
		}
		if (mat->emissiveTexture >= 0) {
			std::string path = TextureManager::Get().GetTexturePath(mat->emissiveTexture);
			data["Textures"]["emissive"] = SimplifyTexturePath(path);
		}
		if (mat->heightTexture >= 0) {
			std::string path = TextureManager::Get().GetTexturePath(mat->heightTexture);
			data["Textures"]["height"] = SimplifyTexturePath(path);
		}

		// 파일명은 name의 마지막 부분 사용
		std::filesystem::path baseDir = "..\\Assets\\Materials\\";
		std::filesystem::path materialPath(mat->name);
		std::string filename = materialPath.stem().string();  // 확장자 제거

		std::filesystem::path fullPath = baseDir / (filename + ".json");

		// 디렉토리가 없으면 생성
		if (!std::filesystem::exists(baseDir))
			std::filesystem::create_directories(baseDir);

		std::ofstream file(fullPath);
		if (file.is_open()) {
			file << data.dump(4);
			file.close();
			std::cout << "[MaterialSystem] Saved: " << fullPath.string() << std::endl;
		}
		else {
			std::cout << "[MaterialSystem] Failed to save: " << fullPath.string() << std::endl;
		}
	}
}