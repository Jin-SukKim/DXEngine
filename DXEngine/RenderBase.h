#pragma once
#include "D3D11Utils.h"

namespace DE {
	class RenderBase
	{
	public:
		RenderBase();
		virtual ~RenderBase();

		virtual bool Initialize(WindowInfo& window);
		virtual void Update();
		virtual void Render();
		void Present();

		void CreateBackBufferRTV();
		// 렌더링하고 싶은 화면 설정
		void SetViewport(const WindowInfo& window);
		// Rasterization을 어떻게 할지 설정
		void InitRS();
		// DepthStencilState 초기화
		void InitDepthStencilState();
		// DepthStencilView Buffer 생성
		void CreateDepthStencilBuffer(const WindowInfo& window);

		ComPtr<ID3D11Device>& GetDevice() {	return m_device; }
		ComPtr<ID3D11DeviceContext>& GetContext() {	return m_context; }

	protected:
		ComPtr<ID3D11Device> m_device;
		ComPtr<ID3D11DeviceContext> m_context;
		ComPtr<IDXGISwapChain> m_swapChain;
		ComPtr<ID3D11RenderTargetView> m_backBufferRTV;

		// Swap-Buffer의 Back Buffer 포맷은 변경해서 사용할 수 있음
		DXGI_FORMAT m_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM; // 32-bit color

		D3D11_VIEWPORT m_screenViewport;
		ComPtr<ID3D11RasterizerState> m_solidRS;
		ComPtr<ID3D11DepthStencilState> m_defaultDSS;
		ComPtr<ID3D11DepthStencilView> m_defaultDSV;
	};
}
