#include "pch.h"
#include "FogEffect.h"
#include "RenderBase.h"

namespace DE {
	void FogEffect::Initialize(RenderBase& renderer, const std::vector<ComPtr<ID3D11ShaderResourceView>>& resources, const std::vector<ComPtr<ID3D11RenderTargetView>>& targets, int width, int height)
	{
		Super::Initialize(renderer, resources, targets, width, height);

		ComPtr<ID3D11Device>& device = renderer.GetDevice();
		ComPtr<ID3D11DeviceContext>& context = renderer.GetContext();
		
		CreateBuffer(device, width, height, m_haloTexture);

		m_haloFilter.Initialize(device, context, RenderBase::graphicsCommon.haloPS, width, height);
		m_haloFilter.SetShaderResources({ resources[0] });
		m_haloFilter.SetRenderTargets({ m_haloTexture.GetRTV() });

		m_fogFilter.Initialize(device, context, RenderBase::graphicsCommon.fogPS, width, height);
		m_fogFilter.SetShaderResources({ m_haloTexture.GetSRV() });
		m_fogFilter.SetRenderTargets(targets);
		
		m_const.Initialize(renderer.GetDevice());
	}
	void FogEffect::Update(ComPtr<ID3D11DeviceContext>& context)
	{
		m_fogFilter.UpdateConstantBuffer(context);
		m_haloFilter.UpdateConstantBuffer(context);
		m_const.Upload(context);
	}
	void FogEffect::Render(RenderBase& renderer)
	{
		Super::Render(renderer);

		ComPtr<ID3D11DeviceContext>& context = renderer.GetContext();
		context->PSSetConstantBuffers(5, 1, m_const.GetAddressOf());

		Texture2D& depthBuffer = renderer.GetDepthOnlyBuffer();
		context->PSSetShaderResources(1, 1, depthBuffer.GetAddressOfSRV());

		RenderImageFilter(context, m_haloFilter);
		RenderImageFilter(context, m_fogFilter);
	}
}