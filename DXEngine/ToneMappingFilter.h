#pragma once
#include "PostProcess.h"

namespace DE {
    class ToneMappingFilter : public PostProcess
    {
		using Super = PostProcess;
	public:
		void Initialize(const std::vector<ComPtr<ID3D11ShaderResourceView>>& resources, const std::vector<ComPtr<ID3D11RenderTargetView>>& targets, int width, int height) override;
		void Update() override;
		void Render() override;
	public:
		struct ToneMappingConsts {
			int useLinear = true;
			int useFilmic = false;
			int useUncharted2 = false;
			int useLumaBasedReinhard = false;
		};

	private:
		ImageFilter m_toneMapping;
		ConstantBuffer<ToneMappingConsts> m_const;
    };
}
