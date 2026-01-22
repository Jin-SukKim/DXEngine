#include "pch.h"
#include "VisualModule.h"
#include "ParticleEmitter.h"

namespace DE {
	void VisualModule::Initialize(ParticleInitContext& ctx)
	{
	}

	void VisualModule::OnSpawn(SimulationContext& ctx)
	{
		ParticleModule::OnSpawn(ctx);
		VisualConsts& consts = ctx.constBuffer.GetCpu().visual;
		consts.startColor = startColor;
		consts.endColor = endColor;
		consts.sizeRange = sizeRange;
		consts.minRotation = minRotation;
		consts.maxRotation = maxRotation;
		consts.minRotSpeed = minRotSpeed;
		consts.maxRotSpeed = maxRotSpeed;
	}

	void VisualModule::LoadFromJson(const json& data)
	{
		if (data.contains("startColor")) startColor = JsonToVec4(data["startColor"]);
		if (data.contains("endColor")) endColor = JsonToVec4(data["endColor"]);
		if (data.contains("sizeRange")) sizeRange = JsonToVec2(data["sizeRange"]);
		if (data.contains("rotation")) {
			auto& rot = data["rotation"];
			if (rot.contains("minRotation")) minRotation = JsonToVec3(rot["minRotation"]);
			if (rot.contains("maxRotation")) maxRotation = JsonToVec3(rot["maxRotation"]);
			if (rot.contains("minRotSpeed")) minRotSpeed = JsonToVec3(rot["minRotSpeed"]);
			if (rot.contains("maxRotSpeed")) maxRotSpeed = JsonToVec3(rot["maxRotSpeed"]);
		}
	}
}