#include "pch.h"
#include "MaterialModule.h"
#include "MaterialSystem.h"
#include "RenderModule.h"
#include "ModelManager.h"

namespace DE {
	void MaterialModule::Initialize(ParticleInitContext& ctx)
	{
		ParticleModule::Initialize(ctx);
	}

	void MaterialModule::OnSpawn(SimulationContext& ctx)
	{
		ParticleModule::OnSpawn(ctx);

		if (!m_isLoadedFromJson && ctx.renderModule) {
			const Model* model = ModelManager::Get().GetModel(ctx.renderModule->GetModelIndex());
			if (model)
				m_materialIndices = model->materialIndices; // 모델의 기본 Material Index 복사
		}
	}

	void MaterialModule::BindMaterialForMesh(int subMeshIndex)
	{
		int matIndex = 0; // Default

		// 유효한 인덱스인지 확인
		if (subMeshIndex >= 0 && subMeshIndex < m_materialIndices.size()) {
			matIndex = m_materialIndices[subMeshIndex];
		}
		else if (!m_materialIndices.empty()) {
			// 인덱스가 범위를 벗어나면 첫 번째 재질이라도 사용 (안전장치)
			matIndex = m_materialIndices[0];
		}

		MaterialSystem::Get().BindMaterial(matIndex);
	}

	void MaterialModule::LoadFromJson(const json& data)
	{
		m_materialIndices.clear();
		m_materialNames.clear();

		if (data.contains("materials") && data["materials"].is_array())
			// 배열로 된 재질 정의 로드
			for (const auto& matData : data["materials"]) 
				LoadMaterialFromJson(matData);
		else
			LoadMaterialFromJson(data);

		m_isLoadedFromJson = true;
	}

	void MaterialModule::LoadMaterialFromJson(const json& data)
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
		m_materialNames.emplace_back(matName);

		int matIdx = MaterialSystem::Get().CreateMaterial(matName, constants, texPaths);
		m_materialIndices.emplace_back(matIdx);
	}
}