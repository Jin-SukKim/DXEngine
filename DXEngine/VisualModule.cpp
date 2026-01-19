#include "pch.h"
#include "VisualModule.h"
#include "ParticleEmitter.h"

namespace DE {
	void VisualModule::Initialize(ParticleInitContext& ctx)
	{
		m_visualConsts.Initialize();
	}

	void VisualModule::OnSpawn(SimulationContext& ctx)
	{
		ParticleModule::OnSpawn(ctx);
		VisualConsts& consts = m_visualConsts.GetCpu();
		consts.startColor = startColor;
		consts.endColor = endColor;
		consts.sizeRange = sizeRange;
		consts.minRotation = minRotation;
		consts.maxRotation = maxRotation;
		consts.minRotSpeed = minRotSpeed;
		consts.maxRotSpeed = maxRotSpeed;
		m_visualConsts.Upload();
		ctx.context->CSSetConstantBuffers(6, 1, m_visualConsts.GetAddressOf());
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