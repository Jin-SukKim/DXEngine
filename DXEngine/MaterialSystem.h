#pragma once
#include "MeshData.h"
#include "Material.h"

namespace DE {

class MaterialSystem
{
public:
	static MaterialSystem& Get() {
		static MaterialSystem instance;
		return instance;
	}
	enum class TexSlot {
		Albedo = 0,
		Normal = 1,
		Metallic = 2,
		Roughness = 3,
		AO = 4,
		Emissive = 5
	};
	void Initialize();
	int CreateMaterial(const std::string& name, const MeshData& meshData, bool isGLTF = false);
	void BindMaterial(int materialIdx);

	void SetTexture(const std::string& matName, TexSlot slot, const std::string& texPath, bool isGLTF = false);
	void SetTexture(int matIdx, TexSlot slot, const std::string& texPath, bool isGLTF = false);

	MaterialConstants& GetMaterialConst(int materialIdx);
	ConstantBuffer<MaterialConstants>& GetMaterialConstBuffer(int materialIdx);
private:
	std::vector<Material> m_materials;
	std::unordered_map<std::string, int> m_materialMap;

	std::vector<ConstantBuffer<MaterialConstants>> m_materialConsts;

};
}

