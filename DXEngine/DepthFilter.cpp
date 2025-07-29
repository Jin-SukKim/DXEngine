#include "pch.h"
#include "DepthFilter.h"
#include "RenderBase.h"

namespace DE {
	void DepthFilter::Initialize(const std::vector<ComPtr<ID3D11ShaderResourceView>>& resources, const std::vector<ComPtr<ID3D11RenderTargetView>>& targets, int width, int height)
	{
		Super::Initialize(resources, targets, width, height);

		m_depthFilter.Initialize(RenderBase::graphicsCommon.depthPS, width, height);
		m_depthFilter.SetShaderResources({ resources[0] });
		m_depthFilter.SetRenderTargets(targets);

		m_const.Initialize();
	}
	void DepthFilter::Update()
	{
		m_depthFilter.UpdateConstantBuffer();
		m_const.Upload();
	}
	void DepthFilter::Render()
	{
		Super::Render();

		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();
		context->PSSetConstantBuffers(5, 1, m_const.GetAddressOf());

		Texture2D& depthBuffer = GET_SINGLE(RenderBase)->GetDepthOnlyBuffer();
		context->PSSetShaderResources(1, 1, depthBuffer.GetAddressOfSRV());

		RenderImageFilter(m_depthFilter);
	}
}