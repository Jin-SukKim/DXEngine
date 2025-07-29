#include "pch.h"
#include "CopyFilter.h"
#include "RenderBase.h"

namespace DE {
	void CopyFilter::Initialize(const std::vector<ComPtr<ID3D11ShaderResourceView>>& resources, const std::vector<ComPtr<ID3D11RenderTargetView>>& targets, int width, int height)
	{
		Super::Initialize(resources, targets, width, height);

		m_copyFilter.Initialize(RenderBase::graphicsCommon.copyPS, width, height);
		m_copyFilter.SetShaderResources({ resources[0] });
		m_copyFilter.SetRenderTargets(targets);
	}

	void CopyFilter::Update()
	{
		m_copyFilter.UpdateConstantBuffer();
	}

	void CopyFilter::Render()
	{
		Super::Render();

		RenderImageFilter(m_copyFilter);
	}
}