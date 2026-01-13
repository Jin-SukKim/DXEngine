#include "pch.h"
#include "RenderModule.h"
#include "ParticleEmitter.h"

namespace DE {
	void RenderModule::Initialize(ParticleInitContext& ctx)
	{
		ParticleModule::Initialize(ctx);

		m_InitSortKeysCS.Initialize(ctx.device, L"InitBitonicSortCS.hlsl");
		m_sort.Initialize(ctx.device, ctx.consts.maxParticles, L"BitonicSortCS.hlsl");
	}

	void RenderModule::OnSpawn(SimulationContext& ctx)
	{
		ParticleModule::OnSpawn(ctx);
		SetBlendState();
	}

	void RenderModule::Draw(const RenderContext& ctx)
	{
		if (!m_isEnabled)
			return;

		ID3D11UnorderedAccessView* uav[1] = { m_sort.GetUAV() };
		ctx.context->CSSetUnorderedAccessViews(0, 1, uav, nullptr);
		ID3D11ShaderResourceView* srvs[] = {
			ctx.particleSRV,
			ctx.countSRV
		};
		ctx.context->CSSetShaderResources(0, 2, srvs);
		m_InitSortKeysCS.Dispatch(ctx.context, (ctx.consts.maxParticles + 1023) / 1024, 1, 1);

		m_sort.Sort(ctx.context);
	}

	void RenderModule::SetBlendState()
	{
		switch (blendMode)
		{
		case BlendMode::Additive:
			m_blendState = RenderBase::graphicsCommon.accumulateBS.Get();
			break;
		case BlendMode::AlphaBlend:
			m_blendState = RenderBase::graphicsCommon.alphaBS.Get();
			break;
		case BlendMode::Opaque:
			m_blendState = nullptr;
			break;
		}
	}

	void RenderModule::LoadFromJson(const json& data)
	{
		if (data.contains("blendMode")) {
			std::string mode = data["blendMode"];
			if (mode == "Additive") blendMode = BlendMode::Additive;
			if (mode == "AlphaBlend") blendMode = BlendMode::AlphaBlend;
			if (mode == "Opaque") blendMode = BlendMode::Opaque;
		}
	}

	void BillboardRenderModule::Draw(const RenderContext& ctx)
	{
		RenderModule::Draw(ctx);

		RenderBase& renderer = *GET_SINGLE(RenderBase);
		renderer.SetPipelineState(RenderBase::graphicsCommon.particle.animPSO);
		ctx.context->OMSetBlendState(m_blendState, RenderBase::graphicsCommon.particle.animPSO.blendFactor, 0xffffffff);
		// IndirectDraw
		ID3D11ShaderResourceView* sortSRVs[] = {
			ctx.particleSRV,
			m_sort.GetSRV()
		};

		ctx.context->VSSetShaderResources(0, 2, sortSRVs);
		ctx.context->DrawInstancedIndirect(ctx.indirectArgsBuffer, 0);

		// Á¤¸®
		ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
		ctx.context->VSSetShaderResources(0, 2, nullSRVs);
	}
}