#include "pch.h"
#include "ImageFilter.h"

namespace DE {
	ImageFilter::ImageFilter(ComPtr<ID3D11PixelShader>& pixelShader, int width, int height)
	{
		Initialize(pixelShader, width, height);
	}

	void ImageFilter::Initialize(ComPtr<ID3D11PixelShader>& pixelShader, int width, int height)
	{
		// Pixel Shader 복사
		ThrowIfFailed(pixelShader.CopyTo(m_pixelShader.GetAddressOf()));

		ZeroMemory(&m_viewport, sizeof(D3D11_VIEWPORT));
		m_viewport.TopLeftX = 0;
		m_viewport.TopLeftY = 0;
		m_viewport.Width = float(width);
		m_viewport.Height = float(height);
		m_viewport.MinDepth = 0.0f;
		m_viewport.MaxDepth = 1.f;

		// Constant Buffer 생성
		m_const.Initialize();
		m_const.GetCpu().dx = 1.f / width;
		m_const.GetCpu().dy = 1.f / height;
	}

	void ImageFilter::UpdateConstantBuffer()
	{
		m_const.Upload();
	}

	void ImageFilter::Render() const
	{
		assert(m_shaderResources.size() > 0);
		assert(m_renderTargets.size() > 0);
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();
		context->RSSetViewports(1, &m_viewport);
		// Image Filter이므로 DepthStencilView는 필요없음
		context->OMSetRenderTargets(UINT(m_renderTargets.size()), m_renderTargets.data(), NULL);
		context->PSSetShader(m_pixelShader.Get(), 0, 0);
		context->PSSetShaderResources(0, UINT(m_shaderResources.size()), m_shaderResources.data());
		context->PSSetConstantBuffers(4, 1, m_const.GetAddressOf());
	}

	void ImageFilter::SetShaderResources(const std::vector<ComPtr<ID3D11ShaderResourceView>>& resources)
	{
		m_shaderResources.clear();
		for (const auto& res : resources)
			m_shaderResources.emplace_back(res.Get());
	}

	void ImageFilter::SetRenderTargets(const std::vector<ComPtr<ID3D11RenderTargetView>>& targets)
	{
		m_renderTargets.clear();
		for (const auto& rtv : targets)
			m_renderTargets.emplace_back(rtv.Get());
	}
}