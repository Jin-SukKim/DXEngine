#include "pch.h"
#include "RenderModule.h"
#include "ParticleEmitter.h"

namespace DE {
	void RenderModule::Initialize(ID3D11Device* device, ParticleEmitter* owner)
	{
		ParticleModule::Initialize(device, owner);

		m_device = device;
		m_InitSortKeysCS.Initialize(device, L"InitBitonicSortCS.hlsl");
		m_sort.Initialize(device, m_owner->GetConsts().maxParticles, L"BitonicSortCS.hlsl");
	}

	void RenderModule::OnSpawn(ID3D11DeviceContext* context)
	{
		SetBlendState();
	}

	void RenderModule::Draw(ID3D11DeviceContext* context, ID3D11Buffer* indirectArgs, ID3D11ShaderResourceView* particleSRV, ID3D11ShaderResourceView* m_countSRV)
	{
		context->CSSetUnorderedAccessViews(0, 1, m_sort.m_array.GetAddressOfUAV(), nullptr);
		ID3D11ShaderResourceView* srvs[] = {
			particleSRV,
			m_countSRV
		};
		context->CSSetShaderResources(0, 2, srvs);
		m_InitSortKeysCS.Dispatch(context, (m_owner->GetConsts().maxParticles + 1023) / 1024, 1, 1);

		m_sort.Sort(context);
	}

	void RenderModule::SetBlendState()
	{
		switch (blendMode)
		{
		case BlendMode::Additive:
			RenderBase::graphicsCommon.particle.animPSO.blendState = RenderBase::graphicsCommon.accumulateBS;
			break;
		case BlendMode::AlphaBlend:
			RenderBase::graphicsCommon.particle.animPSO.blendState = RenderBase::graphicsCommon.alphaBS;
			break;
		case BlendMode::Opaque:
			RenderBase::graphicsCommon.particle.animPSO.blendState = nullptr;
			break;
		}
	}

	void BillboardRenderModule::Draw(ID3D11DeviceContext* context, ID3D11Buffer* indirectArgs, ID3D11ShaderResourceView* particleSRV, ID3D11ShaderResourceView* m_countSRV)
	{
		RenderModule::Draw(context, indirectArgs, particleSRV, m_countSRV);

		RenderBase& renderer = *GET_SINGLE(RenderBase);
		renderer.SetPipelineState(RenderBase::graphicsCommon.particle.animPSO);
		// IndirectDraw
		ID3D11ShaderResourceView* sortSRVs[] = {
			particleSRV,
			m_sort.m_array.GetSRV()
		};

		context->VSSetShaderResources(0, 2, sortSRVs);
		context->DrawInstancedIndirect(indirectArgs, 0);

		// Á¤¸®
		ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
		context->VSSetShaderResources(0, 2, nullSRVs);
	}
}