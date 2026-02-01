#include "pch.h"
#include "OrbitModule.h"
#include "ParticleEmitter.h"

namespace DE {
	void OrbitModule::Initialize(ParticleInitContext& ctx)
	{
		// ConstantBuffer 구조체에 Orbit 관련 필드가 추가되어야 합니다.
		auto& consts = ctx.consts.orbit;
		consts.center = m_center;
		consts.axis = m_axis;
		consts.rotationRate = m_rotationRate; // 셰이더에서 dt와 곱해 사용
		consts.initialOffset = m_initialOffset;
	}

	void OrbitModule::OnUpdate(const SimulationContext& ctx)
	{
		ParticleModule::OnUpdate(ctx);

		ctx.context->CSSetShaderResources(0, 1, ctx.readCount.GetAddressOfSRV());

		UINT initCounts[1] = { static_cast<UINT>(-1) };
		ctx.context->CSSetUnorderedAccessViews(0, 1, ctx.readParticles.GetAddressOfUAV(), initCounts);

		// ComputeCommon에 등록된 orbitCS 사용
		auto& orbitCS = RenderBase::computeCommon.particle.orbitCS;
		ctx.context->CSSetShader(orbitCS.computeShader.Get(), 0, 0);
		ctx.context->DispatchIndirect(ctx.dispatchArgs, ctx.argsOffset);

		// Unbind
		ID3D11ShaderResourceView* nullSRVs[1] = { nullptr };
		ID3D11UnorderedAccessView* nullUAVs[1] = { nullptr };
		ctx.context->CSSetShaderResources(0, 1, nullSRVs);
		ctx.context->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);
		ctx.context->CSSetShader(nullptr, 0, 0);
	}

	void OrbitModule::LoadFromJson(const json& data)
	{
		if (data.contains("center")) m_center = JsonToVec3(data["center"]);
		if (data.contains("axis")) m_axis = JsonToVec3(data["axis"]);
		if (data.contains("rotationRate")) m_rotationRate = data["rotationRate"];
		if (data.contains("offset")) m_initialOffset = data["offset"];
	}

	std::unique_ptr<ParticleModule> OrbitModule::Clone() const
	{
		auto cloned = std::make_unique<OrbitModule>();
		cloned->m_center = this->m_center;
		cloned->m_axis = this->m_axis;
		cloned->m_rotationRate = this->m_rotationRate;
		cloned->m_initialOffset = this->m_initialOffset;
		cloned->m_isEnabled = this->m_isEnabled;
		return cloned;
	}
}