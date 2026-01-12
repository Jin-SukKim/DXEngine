#include "pch.h"
#include "ForceModule.h"
#include "ParticleEmitter.h"

namespace DE {
	void ForceModule::OnSpawn(ID3D11DeviceContext* context)
	{
		ParticleModule::OnSpawn(context);
		ParticleConsts& consts = m_owner->GetConstsData();
		consts.velocity = velocity;
		consts.speedRange = speedRange;
		consts.randomDir = randomDir;
		consts.gravity = gravity;
		consts.drag = drag;
	}
	void ForceModule::LoadFromJson(const json& data)
	{
		if (data.contains("time")) time = data["time"];
		if (data.contains("velocity")) velocity = JsonToVec3(data["velocity"]);
		if (data.contains("speedRange")) speedRange = JsonToVec2(data["speedRange"]);
		if (data.contains("randomDir")) randomDir = JsonToVec3(data["randomDir"]);
		if (data.contains("gravity")) gravity = JsonToVec3(data["gravity"]);
		if (data.contains("drag")) drag = data["drag"];
	}
}