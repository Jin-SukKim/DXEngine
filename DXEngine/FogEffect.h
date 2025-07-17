#pragma once
#include "DepthFilter.h"

namespace DE {
    class FogEffect : public PostProcess
    {
        using Super = PostProcess;
	public:
		void Initialize(RenderBase& renderer, const std::vector<ComPtr<ID3D11ShaderResourceView>>& resources, const std::vector<ComPtr<ID3D11RenderTargetView>>& targets, int width, int height) override;
		void Update(ComPtr<ID3D11DeviceContext>& context) override;
		void Render(RenderBase& renderer) override;

	public:
		struct FogConsts {
			float depthScale = 1.0f;
			float fogColor[3] = { 1.f, 1.f, 1.f };
			float fogStrength = 0.0f;
			float fogMin = 1.f;
			float fogMax = 5.f;
			float dummy;
		};

	protected:
		ImageFilter m_fogFilter;
		ImageFilter m_haloFilter;
		ConstantBuffer<FogConsts> m_const;

		Texture2D m_haloTexture;
	};
}
