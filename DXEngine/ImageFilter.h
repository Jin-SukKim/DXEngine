#pragma once

namespace DE {
	class ImageFilter
	{
	public:
		ImageFilter() {};
		ImageFilter(ComPtr<ID3D11PixelShader>& pixelShader, int width, int height);

		void Initialize(ComPtr<ID3D11PixelShader>& pixelShader, int width, int height);
		void UpdateConstantBuffer();
		void Render() const;

		// Image Filter Pixel Shader에서 사용할 ShaderResourceView 설정
		void SetShaderResources(const std::vector<ComPtr<ID3D11ShaderResourceView>>& resources);
		// Image Filter Pixel Shader에서 사용할 RenderTargetView 설정
		void SetRenderTargets(const std::vector<ComPtr<ID3D11RenderTargetView>>& targets);

	public:
		struct ImageFilterConstData {
			float dx; // Pixel의 x 간격(길이)
			float dy; // Pixel의 y 간격(길이)
			float threshold;
			float strength;
			// 이 옵션들은 exposure, gamma, blur 등의 다른 값으로 응용 될 수 있음
			float option1; 
			float option2;
			float option3;
			float option4;
		};

		ImageFilterConstData& GetConstData() { return m_const.GetCpu(); }
	protected:	
		ComPtr<ID3D11PixelShader> m_pixelShader;
		ConstantBuffer<ImageFilterConstData> m_const;
		D3D11_VIEWPORT m_viewport = {};

		// Do not delete pointers (PostProcessing 클래스에서 ComPtr<>을 가지고 여기선 포인터만 가지고 사용)
		std::vector<ID3D11ShaderResourceView*> m_shaderResources;
		std::vector<ID3D11RenderTargetView*> m_renderTargets;
	};
}
