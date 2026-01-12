#include "pch.h"
#include "VisualModule.h"
#include "ParticleEmitter.h"

namespace DE {
	void VisualModule::OnSpawn(SimulationContext& ctx)
	{
		ParticleModule::OnSpawn(ctx);
		ctx.consts.startColor = startColor;
		ctx.consts.endColor = endColor;
		ctx.consts.sizeRange = sizeRange;
	}
	void VisualModule::LoadFromJson(const json& data)
	{
		if (data.contains("startColor")) startColor = JsonToVec4(data["startColor"]);
		if (data.contains("endColor")) endColor = JsonToVec4(data["endColor"]);
		if (data.contains("sizeRange")) sizeRange = JsonToVec2(data["sizeRange"]);
	}
}