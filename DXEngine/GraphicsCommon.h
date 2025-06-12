#pragma once
#include "GraphicsPSO.h"

namespace DE {
	class GraphicsCommon {
	public:
		void InitCommonStates(ComPtr<ID3D11Device>& device);

	private:
		// 내부적으로 InitCommonStates()에서 사용

		// RasterizerStates 설정
		void initRasterizerStates(ComPtr<ID3D11Device>& device);
		// Depth-Stencil State 설정
		void initDepthStencilStates(ComPtr<ID3D11Device>& device);
		// InputLayout과 Shader 설정
		void initShaders(ComPtr<ID3D11Device>& device);
		// SamplerState 설정
		void initSamplers(ComPtr<ID3D11Device>& device);
		// BlendState 설정
		void initBlendStates(ComPtr<ID3D11Device>& device);
		// GraphcisPSO 설정
		void initPipelineStates(ComPtr<ID3D11Device>& device);

	public:
		struct {
			// Graphcis Pipeline States
			GraphicsPSO solidPSO;
			GraphicsPSO wirePSO;
			GraphicsPSO boundPSO;
			GraphicsPSO tessellationQuadPSO;
		} basic;

		struct {
			GraphicsPSO solidPSO;
		} normal;

		struct {
			GraphicsPSO solidPSO;
			GraphicsPSO wirePSO;
		} skybox;

		struct {
			GraphicsPSO bloomPSO;
		} postProcess;

		struct {
			GraphicsPSO solidPSO;
		} billboard;


		// Shader에서 공통으로 사용할 Sampler
		std::vector<ID3D11SamplerState*> sampleStates;

		// Rasterizer State (CCW : Counter-Clockwise)
		ComPtr<ID3D11RasterizerState> solidRS;
		ComPtr<ID3D11RasterizerState> wireRS;
		ComPtr<ID3D11RasterizerState> postProcessRS;
		ComPtr<ID3D11RasterizerState> solidBothRS; // front and back (cull-none)

		// Depth Stencil State
		ComPtr<ID3D11DepthStencilState> drawDDS; // 일반적(Default)

		// InputLayouts
		ComPtr<ID3D11InputLayout> basicIL;
		ComPtr<ID3D11InputLayout> skyboxIL;
		ComPtr<ID3D11InputLayout> samplingIL;
		ComPtr<ID3D11InputLayout> billboardIL;;

		// Shaders
		ComPtr<ID3D11VertexShader> basicVS;
		ComPtr<ID3D11PixelShader> basicPS;
		ComPtr<ID3D11PixelShader> colorPS;

		ComPtr<ID3D11VertexShader> normalVS;
		ComPtr<ID3D11GeometryShader> normalGS;
		ComPtr<ID3D11PixelShader> normalPS;

		ComPtr<ID3D11VertexShader> skyboxVS;
		ComPtr<ID3D11PixelShader> skyboxPS;

		ComPtr<ID3D11VertexShader> samplingVS;
		ComPtr<ID3D11PixelShader> bloomDownPS;
		ComPtr<ID3D11PixelShader> bloomUpPS;
		ComPtr<ID3D11PixelShader> combinePS;

		ComPtr<ID3D11VertexShader> billboardVS;
		ComPtr<ID3D11GeometryShader> billboardGS;
		ComPtr<ID3D11PixelShader> billboardPS;

		// Tessellation Sample
		ComPtr<ID3D11VertexShader> tessellationQuadVS;
		ComPtr<ID3D11HullShader> tessellationQuadHS;
		ComPtr<ID3D11DomainShader> tessellationQuadDS;
		ComPtr<ID3D11PixelShader> tessellationQuadPS;

		// Sampler
		ComPtr<ID3D11SamplerState> linearWrapSS;
		ComPtr<ID3D11SamplerState> linearClampSS;

		// Blend States
		static ComPtr<ID3D11PixelShader> TreeBillboardPS;
	};

}
