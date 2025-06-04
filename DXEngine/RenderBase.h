#pragma once
#include "D3D11Utils.h"
#include "GraphicsCommon.h"

namespace DE {
	class GraphicsCommon;
	class GraphicsPSO;
	class PostProcess;

	class RenderBase
	{
	public:
		RenderBase();
		virtual ~RenderBase();

		virtual bool Initialize(WindowInfo& window);
		virtual void Update();
		virtual void Render();
		virtual void PostRender();
		void Present();

		void CreateBackBufferRTV();
		// 렌더링하고 싶은 화면 크기에 맞는 Viewport 설정
		void SetViewport(const WindowInfo& window);
		void SetViewport(const float& width, const float& height);
		// Rasterization을 어떻게 할지 설정
		void InitRS();
		// DepthStencilState 초기화
		void InitDepthStencilState();
		// DepthStencilView Buffer 생성
		void CreateDepthStencilBuffer(const WindowInfo& window);

		ComPtr<ID3D11Device>& GetDevice() {	return m_device; }
		ComPtr<ID3D11DeviceContext>& GetContext() {	return m_context; }
		ComPtr<IDXGISwapChain>& GetSwapChain() { return m_swapChain; }
		void ResizeSwapChain(const WindowInfo& window);

		void SetPipelineState(const GraphicsPSO& pso);
		
		void SetPostProcess(PostProcess& postProcess, const GraphicsPSO& pso);

		// 미리 설정해둔 Setting들
		static GraphicsCommon graphicsCommon;
	protected:
		ComPtr<ID3D11Device> m_device;
		ComPtr<ID3D11DeviceContext> m_context;
		ComPtr<IDXGISwapChain> m_swapChain;
		ComPtr<ID3D11RenderTargetView> m_backBufferRTV;
		
		// TODO
		ComPtr<ID3D11Texture2D> m_tempTexture;
		ComPtr<ID3D11ShaderResourceView> m_backBufferSRV; // 임시로 PostProcessing을 위해 SRV 생성
		Texture2D m_prevFrame;

		// Swap-Buffer의 Back Buffer 포맷은 변경해서 사용할 수 있음
		DXGI_FORMAT m_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM; // 32-bit color

		D3D11_VIEWPORT m_screenViewport;
		ComPtr<ID3D11RasterizerState> m_solidRS;
		ComPtr<ID3D11RasterizerState> m_wireRS;
		bool m_drawAsWire = false;

		ComPtr<ID3D11DepthStencilState> m_defaultDSS;
		ComPtr<ID3D11DepthStencilView> m_defaultDSV;

		// TODO: 여러 개의 PostProcess를 사용하려면 Vector를 사용하는게 좋지 않을까?
		PostProcess* m_postProcess;
		GraphicsPSO m_postProcessPSO;
	};
}
