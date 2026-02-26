#pragma once
#include "GraphicsPSO.h"
#include "ComputePSO.h"

namespace DE {
	class GraphicsCommon {
	public:
		void InitCommonStates(ComPtr<ID3D11Device>& device);

	private:
		//  InitCommonStates() 

		// RasterizerStates 
		void initRasterizerStates(ComPtr<ID3D11Device>& device);
		// Depth-Stencil State 
		void initDepthStencilStates(ComPtr<ID3D11Device>& device);
		// InputLayout Shader 
		void initShaders(ComPtr<ID3D11Device>& device);
		// SamplerState 
		void initSamplers(ComPtr<ID3D11Device>& device);
		// BlendState 
		void initBlendStates(ComPtr<ID3D11Device>& device);
		// GraphcisPSO 
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
			GraphicsPSO basicPSO;
			GraphicsPSO copyPSO;
			GraphicsPSO particleCompositePSO;
		} postProcess;

		struct {
			GraphicsPSO solidPSO;
			GraphicsPSO reflectSolidPSO;
		} billboard;

		struct {
			GraphicsPSO stencilMaskPSO;

			GraphicsPSO reflectSolidPSO;
			GraphicsPSO reflectWirePSO;
			GraphicsPSO reflectSkyboxSolidPSO;
			GraphicsPSO reflectSkyboxWirePSO;
			GraphicsPSO reflectBillboardSolidPSO;
			GraphicsPSO reflectEffectSolidPSO;

			GraphicsPSO mirrorBlendSolidPSO;
			GraphicsPSO mirrorBlendWirePSO;
		} mirror;

		struct {
			GraphicsPSO depthOnlyPSO;
		} depth;

		struct {
			GraphicsPSO animPSO;
			GraphicsPSO meshPSO;
			GraphicsPSO billboardInstancedPSO; // GS 없는 인스턴싱 빌보드
		} particle;

		// Shader Sampler
		std::vector<ID3D11SamplerState*> sampleStates;

		// Rasterizer State (CCW : Counter-Clockwise)
		ComPtr<ID3D11RasterizerState> solidRS;
		ComPtr<ID3D11RasterizerState> wireRS;
		ComPtr<ID3D11RasterizerState> postProcessRS;
		// front and back (cull-none)
		ComPtr<ID3D11RasterizerState> solidBothRS; 
		// Counter-Clockwise FrontFace 
		ComPtr<ID3D11RasterizerState> solidCcwRS;
		ComPtr<ID3D11RasterizerState> wireCcwRS; 

		// Depth Stencil State

		// Default
		ComPtr<ID3D11DepthStencilState> drawDSS; 
		// Masking State
		ComPtr<ID3D11DepthStencilState> maskDSS;
		// Stencil Buffer Masking draw
		ComPtr<ID3D11DepthStencilState> drawMaskedDSS; 

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

		// DepthStencilBuffer   
		ComPtr<ID3D11VertexShader> depthOnlyVS; 
		ComPtr<ID3D11PixelShader> depthOnlyPS;

		// Post-Process
		ComPtr<ID3D11VertexShader> samplingVS;
		ComPtr<ID3D11PixelShader> bloomDownPS;
		ComPtr<ID3D11PixelShader> bloomUpPS;
		ComPtr<ID3D11PixelShader> combinePS;
		ComPtr<ID3D11PixelShader> toneMappingPS;
		ComPtr<ID3D11PixelShader> copyPS;
		ComPtr<ID3D11PixelShader> depthPS;
		ComPtr<ID3D11PixelShader> fogPS;
		ComPtr<ID3D11PixelShader> haloPS;

		// Billboard
		ComPtr<ID3D11VertexShader> billboardVS;
		ComPtr<ID3D11GeometryShader> billboardGS;
		ComPtr<ID3D11PixelShader> billboardPS;
		ComPtr<ID3D11PixelShader> TreeBillboardPS;

		// Tessellation Sample
		ComPtr<ID3D11VertexShader> tessellationQuadVS;
		ComPtr<ID3D11HullShader> tessellationQuadHS;
		ComPtr<ID3D11DomainShader> tessellationQuadDS;
		ComPtr<ID3D11PixelShader> tessellationQuadPS;

		// Sampler
		ComPtr<ID3D11SamplerState> linearWrapSS;
		ComPtr<ID3D11SamplerState> linearClampSS;
		ComPtr<ID3D11SamplerState> shadowPointSS;
		ComPtr<ID3D11SamplerState> shadowCompareSS;
		ComPtr<ID3D11SamplerState> pointClampSS;

		// Blend States
		ComPtr<ID3D11BlendState> mirrorBS;

		// Particle System
		ComPtr<ID3D11InputLayout> particleIL;
		ComPtr<ID3D11VertexShader> particleVS;
		ComPtr<ID3D11GeometryShader> particleGS;
		ComPtr<ID3D11PixelShader> particlePS;
		ComPtr<ID3D11PixelShader> particlePbrPS;
		ComPtr<ID3D11BlendState> accumulateBS; 
		ComPtr<ID3D11BlendState> alphaBS;
		ComPtr<ID3D11BlendState> modulateBS;
		ComPtr<ID3D11DepthStencilState> particleDDS;

		ComPtr<ID3D11VertexShader> particleMeshVS;

		// Billboard Instancing (GS 없음)
		ComPtr<ID3D11InputLayout> particleBillboardIL;
		ComPtr<ID3D11VertexShader> particleBillboardVS;
		ComPtr<ID3D11PixelShader> particleCompositePS;

	};

}
