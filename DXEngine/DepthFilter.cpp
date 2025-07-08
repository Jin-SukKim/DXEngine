#include "pch.h"
#include "DepthFilter.h"
#include "RenderBase.h"

namespace DE {
	void DepthFilter::Initialize(RenderBase& renderer, const std::vector<ComPtr<ID3D11ShaderResourceView>>& resources, const std::vector<ComPtr<ID3D11RenderTargetView>>& targets, int width, int height)
	{
		Super::Initialize(renderer, resources, targets, width, height);

		ComPtr<ID3D11Device>& device = renderer.GetDevice();
		ComPtr<ID3D11DeviceContext>& context = renderer.GetContext();

		m_depthFilter.Initialize(device, context, RenderBase::graphicsCommon.depthPS, width, height);
		m_depthFilter.SetShaderResources({ resources[0] });
		m_depthFilter.SetRenderTargets(targets);

		m_const.Initialize(renderer.GetDevice());
	}
	void DepthFilter::Update(ComPtr<ID3D11DeviceContext>& context)
	{
		m_depthFilter.UpdateConstantBuffer(context);
		m_const.Upload(context);
	}
	void DepthFilter::Render(RenderBase& renderer)
	{
		Super::Render(renderer);

		ComPtr<ID3D11DeviceContext>& context = renderer.GetContext();
		context->PSSetConstantBuffers(5, 1, m_const.GetAddressOf());

		Texture2D& depthBuffer = renderer.GetDepthOnlyBuffer();
		context->PSSetShaderResources(1, 1, depthBuffer.GetAddressOfSRV());

		RenderImageFilter(context, m_depthFilter);
	}
}