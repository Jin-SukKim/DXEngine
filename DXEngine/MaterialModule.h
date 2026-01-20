#pragma once
#include "ParticleModule.h"

namespace DE {

class MaterialModule : public ParticleModule
{
public:
	void Initialize(ParticleInitContext& ctx) override;
	void OnSpawn(SimulationContext& ctx) override;
	virtual void OnRender(const RenderContext& ctx) override;

	void LoadFromJson(const json& data) override;
	void SaveToJson(json& data);

	const std::string& GetMaterialName();
	const int& GetMaterialIdx();
	ModulePriority GetPriority() { return ModulePriority::Material; }
private:
	int m_materialIdx = -1;
	std::string m_materialName;

	static std::string presetPath;
};
}

