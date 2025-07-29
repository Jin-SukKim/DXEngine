#pragma once
#include "PostProcess.h"

namespace DE {
	class CopyFilter : public PostProcess
	{
		using Super = PostProcess;
	public:
		void Initialize(const std::vector<ComPtr<ID3D11ShaderResourceView>>& resources, const std::vector<ComPtr<ID3D11RenderTargetView>>& targets, int width, int height) override;
		void Update() override;
		void Render() override;
		
	private:
		ImageFilter m_copyFilter;

	};
}
