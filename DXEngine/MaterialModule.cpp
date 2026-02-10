#include "pch.h"
#include "MaterialModule.h"
#include "MaterialSystem.h"
#include "RenderModule.h"
#include "ModelManager.h"

namespace DE {
	void MaterialModule::Initialize(ParticleInitContext& ctx)
	{
		ParticleModule::Initialize(ctx);

		RenderConsts& consts = ctx.consts.render;
		consts.frameTiles = m_frameTiles;
		consts.frameCount = m_frameCount;
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

		if (data.contains("materials") && data["materials"].is_array())
			// 배열로 된 재질 정의 로드
			for (const auto& matPath : data["materials"])
				m_materialIndices.emplace_back(
					MaterialSystem::Get().CreateMaterialFromJson(matPath));

		if (!m_materialIndices.empty())
			m_isLoadedFromJson = true;

		if (data.contains("sprite")) {
			auto& sprite = data["sprite"];
			if (sprite.contains("frameTiles")) m_frameTiles = JsonToVec2(sprite["frameTiles"]);
			if (sprite.contains("frameCount")) m_frameCount = sprite["frameCount"];
		}
	}
	std::unique_ptr<ParticleModule> MaterialModule::Clone() const
	{
		auto cloned = std::make_unique<MaterialModule>();

		cloned->m_isEnabled = this->m_isEnabled;
		cloned->m_isLoadedFromJson = this->m_isLoadedFromJson;
		cloned->m_materialIndices = this->m_materialIndices;
		cloned->m_frameTiles = this->m_frameTiles;
		cloned->m_frameCount = this->m_frameCount;

		return std::move(cloned);
	}
}