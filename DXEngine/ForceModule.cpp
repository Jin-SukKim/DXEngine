#include "pch.h"
#include "ForceModule.h"
#include "ParticleEmitter.h"

namespace DE {
	void ForceModule::OnSpawn(SimulationContext& ctx)
	{
		ParticleModule::OnSpawn(ctx);
		ctx.consts.velocity = velocity;
		ctx.consts.speedRange = speedRange;
		ctx.consts.randomDir = randomDir;
		ctx.consts.gravity = gravity;
		ctx.consts.drag = drag;
	}
	void ForceModule::LoadFromJson(const json& data)
	{
		if (data.contains("velocity")) velocity = JsonToVec3(data["velocity"]);
		if (data.contains("speedRange")) speedRange = JsonToVec2(data["speedRange"]);
		if (data.contains("randomDir")) randomDir = JsonToVec3(data["randomDir"]);
		if (data.contains("gravity")) gravity = JsonToVec3(data["gravity"]);
		if (data.contains("drag")) drag = data["drag"];
	}
}