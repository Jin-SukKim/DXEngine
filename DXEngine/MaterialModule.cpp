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
			if (textures.contains("height")) texPaths[6] = textures["height"];
		}

		std::string matName = data.value("Name", "Mat_" + std::to_string((uint64_t)this));
		m_materialName = matName;

		// 4. MaterialSystem¿¡ µî·Ï
		m_materialIdx = MaterialSystem::Get().CreateMaterial(matName, constants, texPaths);
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