#pragma once
#include "ImageFilter.h"

namespace DE {
	struct Mesh;

	class PostProcess
	{
	public:
		void Initialize(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, ComPtr<ID3D11PixelShader>& pixelShader, int width, int height);
		void Render(ComPtr<ID3D11DeviceContext>& context, ComPtr<ID3D11SamplerState>& sampler);

		// ImageFilter 렌더링 (PostProcessing할때 한 효과를 위해 여러개의 ImageFilter를 사용하는 경우도 많음)
		void RenderImageFilter(ComPtr<ID3D11DeviceContext>& context, const ImageFilter& imageFilter);
		// SRV와 RTV를 생성하기 위한 함수
		void CreateBuffer(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, int width, int height, Texture2D& texture);
	private:
		std::shared_ptr<Mesh> m_mesh;
	};
}
