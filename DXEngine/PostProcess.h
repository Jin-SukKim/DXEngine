#pragma once
#include "ImageFilter.h"

namespace DE {
	struct Mesh;
	class RenderBase;

	class PostProcess
	{
	public:
		virtual void Initialize(
			const std::vector<ComPtr<ID3D11ShaderResourceView>>& resources,
			const std::vector<ComPtr<ID3D11RenderTargetView>>& targets, int width, int height);
		virtual void Update() {};
		virtual void Render();

		// ImageFilter 렌더링 (PostProcessing할때 한 효과를 위해 여러개의 ImageFilter를 사용하는 경우도 많음)
		void RenderImageFilter(const ImageFilter& imageFilter);
		// SRV와 RTV를 생성하기 위한 함수
		void CreateBuffer(int width, int height, Texture2D& texture);
	private:
		std::shared_ptr<Mesh> m_mesh;
	};
}
