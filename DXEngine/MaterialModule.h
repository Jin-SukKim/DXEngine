#pragma once
#include "ParticleModule.h"

namespace DE {

class MaterialModule : public ParticleModule
{
public:
	void Initialize(ParticleInitContext& ctx) override;
	void OnSpawn(SimulationContext& context) override;

	// 특정 서브 메쉬(SubMesh) 인덱스에 해당하는 재질 바인딩
	void BindMaterialForMesh(int subMeshIndex);

	void LoadFromJson(const json& data) override;

	ModulePriority GetPriority() { return ModulePriority::Material; }
	std::unique_ptr<ParticleModule> Clone() const override;
private:
	// 모델의 각 Mesh에 대응하는 재질 인덱스들
	std::vector<int> m_materialIndices;

	// JSON에서 로드된 재질 이름들 (저장/복원용)
	bool m_isLoadedFromJson = false;
};
}

