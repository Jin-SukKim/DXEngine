#include "pch.h"
#include "MaterialModule.h"
#include "MaterialSystem.h"

namespace DE {
	std::string MaterialModule::presetPath = "..\\Assets\\Materials\\";

	void MaterialModule::Initialize(ParticleInitContext& ctx)
	{
		ParticleModule::Initialize(ctx);
	}

	void MaterialModule::OnSpawn(SimulationContext& ctx)
	{
		ParticleModule::OnSpawn(ctx);
	}

	void MaterialModule::OnRender(const RenderContext& ctx)
	{
		ParticleModule::OnRender(ctx);
		MaterialSystem::Get().BindMaterial(m_materialIdx);
	}

	void MaterialModule::LoadFromJson(const json& data)
	{
		MaterialConstants constants;

		if (data.contains("Albedo"))
			constants.albedoFactor = JsonToVec3(data["Albedo"]);
		if (data.contains("Roughness"))
			constants.roughnessFactor = data["Roughness"];
		if (data.contains("Metallic"))
			constants.metallicFactor = data["Metallic"];
		if (data.contains("Emission"))
			constants.emissionFactor = JsonToVec3(data["Emission"]);
		std::vector<std::string> texPaths(6);

		if (data.contains("Textures")) {
			auto& textures = data["Textures"];
			if (textures.contains("albedo")) texPaths[0] = textures["albedo"];
			if (textures.contains("normal")) texPaths[1] = textures["normal"];
			if (textures.contains("metallic")) texPaths[2] = textures["metallic"];
			if (textures.contains("roughness")) texPaths[3] = textures["roughness"];
			if (textures.contains("ao")) texPaths[4] = textures["ao"];
			if (textures.contains("emissive")) texPaths[5] = textures["emissive"];
		}

		std::string matName = data.value("Name", "Mat_" + std::to_string((uint64_t)this));
		m_materialName = matName;

		// 4. MaterialSystem에 등록
		m_materialIdx = MaterialSystem::Get().CreateMaterial(matName, constants, texPaths);
	}

	void MaterialModule::SaveToJson(json& data)
	{
		const MaterialConstants& constants = MaterialSystem::Get().GetMaterialConst(m_materialIdx);

		json matData;
		matData["Name"] = m_materialName;

		// Constants 저장
		matData["Albedo"] = { constants.albedoFactor.x, constants.albedoFactor.y, constants.albedoFactor.z };
		matData["Roughness"] = constants.roughnessFactor;
		matData["Metallic"] = constants.metallicFactor;
		matData["Emission"] = { constants.emissionFactor.x, constants.emissionFactor.y, constants.emissionFactor.z };

		// Textures 저장 (경로 복원은 TextureManager 구조에 따라 다를 수 있음)
		// TextureManager가 경로를 캐싱하고 있다고 가정
		json texData;
		// TextureManager::Get().GetTexturePath(index) 같은 함수가 필요할 수 있음
		// 여기서는 생략, 로직 구현 필요

		data = matData;
	}

	const std::string& MaterialModule::GetMaterialName()
	{
		return m_materialName;
	}

	const int& MaterialModule::GetMaterialIdx()
	{
		return m_materialIdx;
	}

}