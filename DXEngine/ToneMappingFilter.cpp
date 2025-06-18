#include "pch.h"
#include "ToneMappingFilter.h"
#include "RenderBase.h"
#include "GraphicsCommon.h"

namespace DE {
	void ToneMappingFilter::Initialize(RenderBase& renderer, const std::vector<ComPtr<ID3D11ShaderResourceView>>& resources, const std::vector<ComPtr<ID3D11RenderTargetView>>& targets, int width, int height)
	{
		Super::Initialize(renderer, resources, targets, width, height);

		m_toneMapping.Initialize(renderer.GetDevice(), renderer.GetContext(), RenderBase::graphicsCommon.toneMappingPS, width, height);
		m_toneMapping.SetShaderResources({ resources[0] });
		m_toneMapping.SetRenderTargets(targets);

		m_toneMapping.GetConstData().strength = 0.3f; // Bloom Strength;
		m_toneMapping.GetConstData().option1 = 1.0f;  // Exposure로 사용;
		m_toneMapping.GetConstData().option2 = 2.2f; // Gamma로 사용;

		m_toneMapping.UpdateConstantBuffer(renderer.GetContext());

		// Constant Buffer 생성
		m_const.Initialize(renderer.GetDevice());
	}
	
	void ToneMappingFilter::Update(ComPtr<ID3D11DeviceContext>& context)
	{
		m_toneMapping.UpdateConstantBuffer(context);
		m_const.Upload(context);
	}

	void ToneMappingFilter::Render(RenderBase& renderer)
	{
		Super::Render(renderer);

		ComPtr<ID3D11DeviceContext>& context = renderer.GetContext();
		// 화면 렌더링
		context->PSSetConstantBuffers(1, 1, m_const.GetAddressOf());
		RenderImageFilter(context, m_toneMapping);
	}
}