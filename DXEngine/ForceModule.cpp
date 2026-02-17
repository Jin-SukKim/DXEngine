#include "pch.h"
#include "ForceModule.h"
#include "ParticleEmitter.h"

namespace DE {
	void ForceModule::Initialize(ParticleInitContext& ctx)
	{
		// ��� �� �ʱ�ȭ (OnSpawn���� �̵�)
		ForceConsts& consts = ctx.consts.force;
		consts.velocity = velocity;
		consts.speedRange = speedRange;
		consts.randomDir = randomDir;
		consts.gravity = gravity;
		consts.drag = drag;
		consts.curlNoiseFrequency = curlNoiseFrequency;
		consts.curlNoiseStrength = curlNoiseStrength;
		consts.curlNoiseEnabled = curlNoiseEnabled ? 1 : 0;
	}

	void ForceModule::OnUpdate(const SimulationContext& ctx)
	{
		ParticleModule::OnUpdate(ctx);

	}

	void ForceModule::LoadFromJson(const json& data)
	{
		if (data.contains("velocity")) velocity = JsonToVec3(data["velocity"]);
		if (data.contains("speedRange")) speedRange = JsonToVec2(data["speedRange"]);
		if (data.contains("randomDir")) randomDir = JsonToVec3(data["randomDir"]);
		if (data.contains("gravity")) gravity = JsonToVec3(data["gravity"]);
		if (data.contains("drag")) drag = data["drag"];
		if (data.contains("curlNoiseFrequency")) curlNoiseFrequency = data["curlNoiseFrequency"];
		if (data.contains("curlNoiseStrength")) curlNoiseStrength = data["curlNoiseStrength"];
		if (data.contains("curlNoiseEnabled")) curlNoiseEnabled = data["curlNoiseEnabled"];
	}

	std::unique_ptr<ParticleModule> ForceModule::Clone() const
	{
		auto cloned = std::make_unique<ForceModule>();

		cloned->velocity = this->velocity;
		cloned->speedRange = this->speedRange;
		cloned->randomDir = this->randomDir;
		cloned->drag = this->drag;
		cloned->gravity = this->gravity;
		cloned->curlNoiseFrequency = this->curlNoiseFrequency;
		cloned->curlNoiseStrength = this->curlNoiseStrength;
		cloned->curlNoiseEnabled = this->curlNoiseEnabled;
		cloned->m_isEnabled = this->m_isEnabled;

		return cloned;
	}
}