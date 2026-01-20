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
		
		// Billboard용 기본 Material 바인딩
		if (m_materialIdx >= 0) {
			MaterialSystem::Get().BindMaterial(m_materialIdx);
		}
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
		
		std::vector<std::string> texPaths(7);

		if (data.contains("Textures")) {
			auto& textures = data["Textures"];
			
			// JSON에서 로드할 때 "Models/" 접두사 자동 추가
			if (textures.contains("albedo")) {
				std::string path = textures["albedo"];
				texPaths[0] = "Models/" + path;
			}
			if (textures.contains("normal")) {
				std::string path = textures["normal"];
				texPaths[1] = "Models/" + path;
			}
			if (textures.contains("metallic")) {
				std::string path = textures["metallic"];
				texPaths[2] = "Models/" + path;
			}
			if (textures.contains("roughness")) {
				std::string path = textures["roughness"];
				texPaths[3] = "Models/" + path;
			}
			if (textures.contains("ao")) {
				std::string path = textures["ao"];
				texPaths[4] = "Models/" + path;
			}
			if (textures.contains("emissive")) {
				std::string path = textures["emissive"];
				texPaths[5] = "Models/" + path;
			}
			if (textures.contains("height")) {
				std::string path = textures["height"];
				texPaths[6] = "Models/" + path;
			}
		}

		std::string matName = data.value("Name", "Mat_" + std::to_string((uint64_t)this));
		m_materialName = matName;

		m_materialIdx = MaterialSystem::Get().CreateMaterial(matName, constants, texPaths);
	}

	const std::string& MaterialModule::GetMaterialName() const
	{
		return m_materialName;
	}

	int MaterialModule::GetMaterialIdx() const
	{
		return m_materialIdx;
	}

	void MaterialModule::BindMaterialByIdx(int materialIdx) const
	{
		MaterialSystem::Get().BindMaterial(materialIdx);
	}
}