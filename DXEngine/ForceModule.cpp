#include "pch.h"
#include "ForceModule.h"
#include "ParticleEmitter.h"

namespace DE {
	void ForceModule::Initialize(ParticleInitContext& ctx)
	{
		// ComputeShader는 ComputeCommon에서 공유
	}

	void ForceModule::OnSpawn(SimulationContext& ctx)
	{
		ParticleModule::OnSpawn(ctx);
		ForceConsts& consts = ctx.constBuffer.GetCpu().force;
		consts.velocity = velocity;
		consts.speedRange = speedRange;
		consts.randomDir = randomDir;
		consts.gravity = gravity;
		consts.drag = drag;
	}

	void ForceModule::OnUpdate(const SimulationContext& ctx)
	{
		ParticleModule::OnUpdate(ctx);
		
		ID3D11ShaderResourceView* srvs[] = { ctx.countSRV };
		ctx.context->CSSetShaderResources(0, 1, srvs);

		UINT initCounts[2] = { static_cast<UINT>(-1), 0 };
		ID3D11UnorderedAccessView* particleUAVs[] = {
			ctx.consumeBuffer.GetUAV(),
			ctx.appendBuffer.GetUAV()
		};

		ctx.context->CSSetUnorderedAccessViews(0, 2, particleUAVs, initCounts);

		// ComputeCommon의 공유 ComputePSO 사용
		auto& particleCS = RenderBase::computeCommon.particle.particleCS;
		ctx.context->CSSetShader(particleCS.computeShader.Get(), 0, 0);
		ctx.context->DispatchIndirect(ctx.dispatchArgs, 0);
		
		// Barrier
		ID3D11ShaderResourceView* nullSRVs[1] = { nullptr };
		ID3D11UnorderedAccessView* nullUAVs[2] = { nullptr, nullptr };
		ctx.context->CSSetShaderResources(0, 1, nullSRVs);
		ctx.context->CSSetUnorderedAccessViews(0, 2, nullUAVs, nullptr);
		ctx.context->CSSetShader(nullptr, 0, 0);
	}

	void ForceModule::LoadFromJson(const json& data)
	{
		if (data.contains("velocity")) velocity = JsonToVec3(data["velocity"]);
		if (data.contains("speedRange")) speedRange = JsonToVec2(data["speedRange"]);
		if (data.contains("randomDir")) randomDir = JsonToVec3(data["randomDir"]);
		if (data.contains("gravity")) gravity = JsonToVec3(data["gravity"]);
		if (data.contains("drag")) drag = data["drag"];
	}

	std::unique_ptr<ParticleModule> ForceModule::Clone() const
	{
		auto cloned = std::make_unique<ForceModule>();

		cloned->velocity = this->velocity;
		cloned->speedRange = this->speedRange;
		cloned->randomDir = this->randomDir;
		cloned->drag = this->drag;
		cloned->gravity = this->gravity;
		cloned->m_isEnabled = this->m_isEnabled;

		return cloned;
	}
}