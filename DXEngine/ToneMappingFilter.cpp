#include "pch.h"
#include "ToneMappingFilter.h"
#include "RenderBase.h"
#include "GraphicsCommon.h"

namespace DE {
	void ToneMappingFilter::Initialize(const std::vector<ComPtr<ID3D11ShaderResourceView>>& resources, const std::vector<ComPtr<ID3D11RenderTargetView>>& targets, int width, int height)
	{
		Super::Initialize(resources, targets, width, height);

		m_toneMapping.Initialize(RenderBase::graphicsCommon.toneMappingPS, width, height);
		m_toneMapping.SetShaderResources({ resources[0] });
		m_toneMapping.SetRenderTargets(targets);

		m_toneMapping.GetConstData().strength = 0.3f; // Bloom Strength;
		m_toneMapping.GetConstData().option1 = 1.0f;  // Exposure로 사용;
		m_toneMapping.GetConstData().option2 = 2.2f; // Gamma로 사용;

		m_toneMapping.UpdateConstantBuffer();

		// Constant Buffer 생성
		m_const.Initialize();
	}
	
	void ToneMappingFilter::Update()
	{
		m_toneMapping.UpdateConstantBuffer();
		m_const.Upload();
	}

	void ToneMappingFilter::Render()
	{
		Super::Render();

		// 화면 렌더링
		GET_SINGLE(RenderBase)->GetContext()->PSSetConstantBuffers(5, 1, m_const.GetAddressOf());
		RenderImageFilter(m_toneMapping);
	}
}