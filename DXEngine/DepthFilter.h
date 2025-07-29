#pragma once
#include "PostProcess.h"

namespace DE {
    class DepthFilter : public PostProcess
    {
		using Super = PostProcess;
	public:
		void Initialize(const std::vector<ComPtr<ID3D11ShaderResourceView>>& resources, const std::vector<ComPtr<ID3D11RenderTargetView>>& targets, int width, int height) override;
		void Update() override;
		void Render() override;
	
	public:
		struct DepthConsts {
			float depthScale = 0.2f;
			float dummy[3];
		};

	protected:
		ImageFilter m_depthFilter;
		ConstantBuffer<DepthConsts> m_const;
	};
}
