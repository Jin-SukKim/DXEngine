#include "pch.h"
#include "CopyFilter.h"
#include "RenderBase.h"

namespace DE {
	void CopyFilter::Initialize(RenderBase& renderer, const std::vector<ComPtr<ID3D11ShaderResourceView>>& resources, const std::vector<ComPtr<ID3D11RenderTargetView>>& targets, int width, int height)
	{
		Super::Initialize(renderer, resources, targets, width, height);

		ComPtr<ID3D11Device>& device = renderer.GetDevice();
		ComPtr<ID3D11DeviceContext>& context = renderer.GetContext();

		m_copyFilter.Initialize(device, context, RenderBase::graphicsCommon.copyPS, width, height);
		m_copyFilter.SetShaderResources({ resources[0] });
		m_copyFilter.SetRenderTargets(targets);
	}

	void CopyFilter::Update(ComPtr<ID3D11DeviceContext>& context)
	{
		m_copyFilter.UpdateConstantBuffer(context);
	}

	void CopyFilter::Render(RenderBase& renderer)
	{
		Super::Render(renderer);

		RenderImageFilter(renderer.GetContext(), m_copyFilter);
	}
}