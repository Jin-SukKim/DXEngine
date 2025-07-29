#include "pch.h"
#include "FogEffect.h"
#include "RenderBase.h"

namespace DE {
	void FogEffect::Initialize(const std::vector<ComPtr<ID3D11ShaderResourceView>>& resources, const std::vector<ComPtr<ID3D11RenderTargetView>>& targets, int width, int height)
	{
		Super::Initialize(resources, targets, width, height);
		
		CreateBuffer(width, height, m_haloTexture);

		m_haloFilter.Initialize(RenderBase::graphicsCommon.haloPS, width, height);
		m_haloFilter.SetShaderResources({ resources[0] });
		m_haloFilter.SetRenderTargets({ m_haloTexture.GetRTV() });

		m_fogFilter.Initialize(RenderBase::graphicsCommon.fogPS, width, height);
		m_fogFilter.SetShaderResources({ m_haloTexture.GetSRV() });
		m_fogFilter.SetRenderTargets(targets);
		
		m_const.Initialize();
	}
	void FogEffect::Update()
	{
		m_fogFilter.UpdateConstantBuffer();
		m_haloFilter.UpdateConstantBuffer();
		m_const.Upload();
	}
	void FogEffect::Render()
	{
		Super::Render();

		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();
		context->PSSetConstantBuffers(5, 1, m_const.GetAddressOf());

		Texture2D& depthBuffer = GET_SINGLE(RenderBase)->GetDepthOnlyBuffer();
		context->PSSetShaderResources(1, 1, depthBuffer.GetAddressOfSRV());

		RenderImageFilter(m_haloFilter);
		RenderImageFilter(m_fogFilter);
	}
}