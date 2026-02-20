#pragma once
#include "ParticleModule.h"

namespace DE {

class MaterialModule : public ParticleModule
{
public:
	void Initialize(ParticleInitContext& ctx) override;

	// Model은 SubMesh로 이루어져 있음 (TODO: 현재는 1개의 mesh만 있는 static mesh만 가능)
	void BindMaterialForMesh(int subMeshIndex);

	void LoadFromJson(const json& data) override;

	ModulePriority GetPriority() { return ModulePriority::Material; }
	std::unique_ptr<ParticleModule> Clone() const override;
	int GetMaterialIndex() { return m_materialIndices[0]; }
private:
	// 각 Mesh의 Material Index
	std::vector<int> m_materialIndices;

	// Json에서 Load해오는 data
	bool m_isLoadedFromJson = false;
	Vector2 m_frameTiles = { 1, 1 };
	UINT m_frameCount = 1;
	float m_animDuration = 1.0f;  // Animation duration (0.0~1.0, default 1.0 = full lifetime)
	bool m_frameBlending = false;
	float m_animTime = 0.0f;
};
}

