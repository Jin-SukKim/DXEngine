#pragma once

namespace DE {
	// 참고: DirectX_Graphic-Samples 미니엔진
	// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/PipelineState.h

	// 참고: D3D12_GRAPHICS_PIPELINE_STATE_DESC
	// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_graphics_pipeline_state_desc

	// PipelineStateObject: 렌더링할 때 Context의 상태를 어떻게 설정해줄지 저장
	// ComputePSO는 별도로 정의
	class GraphicsPSO
	{
	public:
		// 미리 정의해둔 GraphicsPSO를 빠르게 설정할 수 있게 override
		void operator=(const GraphicsPSO& pso);
		void SetBlendFactor(const float blendFactor[4]);

	public:
		ComPtr<ID3D11InputLayout> inputLayout;
		ComPtr<ID3D11VertexShader> vertexShader;
		ComPtr<ID3D11PixelShader> pixelShader;
		ComPtr<ID3D11HullShader> hullShader;
		ComPtr<ID3D11DomainShader> domainShader;
		ComPtr<ID3D11GeometryShader> geometryShader;

		ComPtr<ID3D11BlendState> blendState;
		ComPtr<ID3D11DepthStencilState> depthStencilState;
		ComPtr<ID3D11RasterizerState> rasterizerState;

		// 두 색을 얼마나 섞어줄지에 대한 비율값 (비율은 [0.0, 1.0] 범위)
		float blendFactor[4] = { 1.f, 1.f, 1.f, 1.f };
		UINT stencilRef = 0; // Stencil Buffer에 값을 Write할때 어떤 값을 쓸지

		D3D11_PRIMITIVE_TOPOLOGY primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	};
}
