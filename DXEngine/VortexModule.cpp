#include "pch.h"
#include "VortexModule.h"
#include "ParticleEmitter.h"

namespace DE {
	void VortexModule::Initialize(ParticleInitContext& ctx)
	{
		m_vortexCS.Initialize(ctx.device, L"VortexCS.hlsl");
	}

	void VortexModule::OnSpawn(SimulationContext& ctx)
	{
		ParticleModule::OnSpawn(ctx);
		VortexConsts& consts = ctx.constBuffer.GetCpu().vortex;
		consts.vortexCenter = m_vortexCenter;
		consts.vortexStrength = m_vortexStrength;
		consts.vortexAxis = m_vortexAxis;
		consts.vortexFalloff = m_vortexFalloff;
		consts.vortexPull = m_vortexPull;
	}

	void VortexModule::OnUpdate(const SimulationContext& ctx)
	{
		ParticleModule::OnUpdate(ctx);
		// Counter buffer binding
		ID3D11ShaderResourceView* srvs[] = { ctx.countSRV };
		ctx.context->CSSetShaderResources(0, 1, srvs);

		// UAV 설정 (초기 카운트 지정)
		// -1: consume 버퍼의 기존 카운트 유지
		// 0: append 버퍼의 카운트 리셋
		UINT initCounts[1] = { static_cast<UINT>(-1) };

		ctx.context->CSSetUnorderedAccessViews(0, 1, ctx.consumeBuffer.GetAddressOfRWuav(), initCounts);
		
		// Particle Simulation Compute Shader
		m_vortexCS.DispatchIndirect(ctx.context, ctx.dispatchArgs);
	}

	void VortexModule::LoadFromJson(const json& data)
	{
		if (data.contains("center")) m_vortexCenter = JsonToVec3(data["center"]);
		if (data.contains("strength")) m_vortexStrength = data["strength"];
		if (data.contains("axis")) m_vortexAxis = JsonToVec3(data["axis"]);
		if (data.contains("vortexFalloff")) m_vortexFalloff = data["vortexFalloff"];
		if (data.contains("pull")) m_vortexPull = JsonToVec2(data["pull"]);
	}
}