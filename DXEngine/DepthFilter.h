#pragma once
#include "PostProcess.h"

namespace DE {
    class DepthFilter : public PostProcess
    {
		using Super = PostProcess;
	public:
		void Initialize(RenderBase& renderer, const std::vector<ComPtr<ID3D11ShaderResourceView>>& resources, const std::vector<ComPtr<ID3D11RenderTargetView>>& targets, int width, int height) override;
		void Update(ComPtr<ID3D11DeviceContext>& context) override;
		void Render(RenderBase& renderer) override;
	
	public:
		struct DepthConsts {
			float depthScale = 0.2f;
			float dummy[3];
		};

	private:
		ImageFilter m_depthFilter;
		ConstantBuffer<DepthConsts> m_const;
	};
}
