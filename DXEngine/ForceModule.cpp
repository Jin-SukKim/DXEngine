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
		ctx.consts.vortexCenter = vortexCenter;
		ctx.consts.vortexStrength = vortexStrength;
		ctx.consts.vortexAxis = vortexAxis;
		ctx.consts.vortexPull = vortexPull;
	}
	void ForceModule::LoadFromJson(const json& data)
	{
		if (data.contains("velocity")) velocity = JsonToVec3(data["velocity"]);
		if (data.contains("speedRange")) speedRange = JsonToVec2(data["speedRange"]);
		if (data.contains("randomDir")) randomDir = JsonToVec3(data["randomDir"]);
		if (data.contains("gravity")) gravity = JsonToVec3(data["gravity"]);
		if (data.contains("drag")) drag = data["drag"];
		if (data.contains("vortex")) {
			auto& vortex = data["vortex"];
			if (vortex.contains("center")) vortexCenter = JsonToVec3(vortex["center"]);
			if (vortex.contains("strength")) vortexStrength = vortex["strength"];
			if (vortex.contains("axis")) vortexAxis = JsonToVec3(vortex["axis"]);
			if (vortex.contains("pull")) vortexPull = vortex["pull"];
		}
	}
}