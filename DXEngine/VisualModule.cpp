#include "pch.h"
#include "VisualModule.h"
#include "ParticleEmitter.h"

namespace DE {
	void VisualModule::OnSpawn(ID3D11DeviceContext* context)
	{
		ParticleModule::OnSpawn(context);
		ParticleConsts& consts = m_owner->GetConstsData();
		consts.startColor = startColor;
		consts.endColor = endColor;
		consts.sizeRange = sizeRange;
	}
	void VisualModule::LoadFromJson(const json& data)
	{
		if (data.contains("startColor")) startColor = JsonToVec4(data["startColor"]);
		if (data.contains("endColor")) endColor = JsonToVec4(data["endColor"]);
		if (data.contains("sizeRange")) sizeRange = JsonToVec2(data["sizeRange"]);
	}
}