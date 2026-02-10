#pragma once
#include "ParticleModule.h"

namespace DE {

class MaterialModule : public ParticleModule
{
public:
	void Initialize(ParticleInitContext& ctx) override;

	// Ư�� ���� �޽�(SubMesh) �ε����� �ش��ϴ� ���� ���ε�
	void BindMaterialForMesh(int subMeshIndex);

	void LoadFromJson(const json& data) override;

	// Get material index for a specific submesh
	int GetMaterialIndex(int subMeshIndex) const {
		if (subMeshIndex >= 0 && subMeshIndex < m_materialIndices.size()) {
			return m_materialIndices[subMeshIndex];
		}
		else if (!m_materialIndices.empty()) {
			return m_materialIndices[0];
		}
		return 0; // Default material
	}

	ModulePriority GetPriority() { return ModulePriority::Material; }
	std::unique_ptr<ParticleModule> Clone() const override;
private:
	// ���� �� Mesh�� �����ϴ� ���� �ε�����
	std::vector<int> m_materialIndices;

	// JSON���� �ε�� ���� �̸��� (����/������)
	bool m_isLoadedFromJson = false;
	Vector2 m_frameTiles = { 1, 1 };
	UINT m_frameCount = 1;
};
}

