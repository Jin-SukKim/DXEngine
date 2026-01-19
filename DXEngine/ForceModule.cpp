#include "pch.h"
#include "ForceModule.h"
#include "ParticleEmitter.h"

namespace DE {
	void ForceModule::Initialize(ParticleInitContext& ctx)
	{
		m_forceConsts.Initialize();
		m_particleCS.Initialize(ctx.device, L"ParticleCS.hlsl");
	}

	void ForceModule::OnSpawn(SimulationContext& ctx)
	{
		ParticleModule::OnSpawn(ctx);
		ForceConsts& consts = m_forceConsts.GetCpu();
		consts.velocity = velocity;
		consts.speedRange = speedRange;
		consts.randomDir = randomDir;
		consts.gravity = gravity;
		consts.drag = drag;
		m_forceConsts.Upload(); 
		ctx.context->CSSetConstantBuffers(7, 1, m_forceConsts.GetAddressOf());
	}

	void ForceModule::OnUpdate(const SimulationContext& ctx)
	{
		ParticleModule::OnUpdate(ctx);
		// Counter buffer binding
		ID3D11ShaderResourceView* srvs[] = { ctx.countSRV };
		ctx.context->CSSetShaderResources(0, 1, srvs);

		// UAV 설정 (초기 카운트 지정)
		// -1: consume 버퍼의 기존 카운트 유지
		// 0: append 버퍼의 카운트 리셋
		UINT initCounts[2] = { static_cast<UINT>(-1), 0 };
		ID3D11UnorderedAccessView* particleUAVs[] = {
			ctx.consumeBuffer.GetUAV(),
			ctx.appendBuffer.GetUAV()
		};

		ctx.context->CSSetUnorderedAccessViews(0, 2, particleUAVs, initCounts);

		// Particle Simulation Compute Shader
		m_particleCS.DispatchIndirect(ctx.context, ctx.dispatchArgs);
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