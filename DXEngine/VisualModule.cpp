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
		ctx.consts.minRotation = minRotation;
		ctx.consts.maxRotation = maxRotation;
		ctx.consts.minRotSpeed = minRotSpeed;
		ctx.consts.maxRotSpeed = maxRotSpeed;

	}
	void VisualModule::LoadFromJson(const json& data)
	{
		if (data.contains("startColor")) startColor = JsonToVec4(data["startColor"]);
		if (data.contains("endColor")) endColor = JsonToVec4(data["endColor"]);
		if (data.contains("sizeRange")) sizeRange = JsonToVec2(data["sizeRange"]);
		if (data.contains("Rotation")) {
			auto& rot = data["Rotation"];
			if (rot.contains("minRotation")) minRotation = JsonToVec3(rot["minRotation"]);
			if (rot.contains("maxRotation")) maxRotation = JsonToVec3(rot["maxRotation"]);
			if (rot.contains("minRotSpeed")) minRotSpeed = JsonToVec3(rot["minRotSpeed"]);
			if (rot.contains("maxRotSpeed")) maxRotSpeed = JsonToVec3(rot["maxRotSpeed"]);
		}
	}
}