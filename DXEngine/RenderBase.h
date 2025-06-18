#pragma once
#include "D3D11Utils.h"
#include "GraphicsCommon.h"

namespace DE {
	class GraphicsCommon;
	class GraphicsPSO;
	class PostProcess;
	class ToneMappingFilter;

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

		// HDR Pipeline에 필요한 Buffer들 생성
		void CreateBuffers();
		// 렌더링하고 싶은 화면 크기에 맞는 Viewport 설정
		void SetViewport(const WindowInfo& window);
		void SetViewport(const float& width, const float& height);
		// DepthStencilView Buffer 생성
		void CreateDepthStencilBuffer(const WindowInfo& window);

		ComPtr<ID3D11Device>& GetDevice() {	return m_device; }
		ComPtr<ID3D11DeviceContext>& GetContext() {	return m_context; }
		ComPtr<IDXGISwapChain>& GetSwapChain() { return m_swapChain; }
		void ResizeSwapChain(const WindowInfo& window);

		void SetPipelineState(const GraphicsPSO& pso);
		
		void SetPostProcess(PostProcess& postProcess, const GraphicsPSO& pso);

		void CopyIndexForPicking(int mouseX, int mouseY, uint8_t* dest);

		// 미리 설정해둔 Setting들
		static GraphicsCommon graphicsCommon;
	protected:
		UINT m_numQualityLevels = 0;
		ComPtr<ID3D11Device> m_device;
		ComPtr<ID3D11DeviceContext> m_context;
		ComPtr<IDXGISwapChain> m_swapChain;
		ComPtr<ID3D11RenderTargetView> m_backBufferRTV;

		// 삼각형 레스터화 -> float(MSAA) -> Resolved(No MSAA) -> Post-Process -> BackBuffer(최종 Swap-Chain Present)
		Texture2D m_floatBuffer;
		Texture2D m_resolvedBuffer;
		
		// Picking
		ComPtr<ID3D11Texture2D> m_indexTempTexture;
		ComPtr<ID3D11Texture2D> m_indexTexture; // Picking을 위한 Index를 저장할 Texture
		ComPtr<ID3D11RenderTargetView> m_indexRTV; 
		ComPtr<ID3D11Texture2D> m_indexStagingTexture; // Picking을 하면 가져올 1x1 pixel data

		// TODO
		//ComPtr<ID3D11Texture2D> m_tempTexture;
		//ComPtr<ID3D11ShaderResourceView> m_backBufferSRV; // 임시로 PostProcessing을 위해 SRV 생성
		Texture2D m_prevFrame;

		// Swap-Buffer의 Back Buffer 포맷은 변경해서 사용할 수 있음
		DXGI_FORMAT m_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM; // 32-bit color (Low Dynamic Range Image)
		// [0.0, 1.0]으로 정해진 범위가 아닌 float으로 더 넓은 범위에 대해서 렌더링을 할 수 있음
		//DXGI_FORMAT m_backBufferFormat = DXGI_FORMAT_R16G16B16A16_FLOAT; // 64-bit color (HDR Pipeline 사용)

		D3D11_VIEWPORT m_screenViewport;
		bool m_drawAsWire = false;

		// Depth-Stencil Buffer
		ComPtr<ID3D11DepthStencilView> m_defaultDSV;

		// TODO: 여러 개의 PostProcess를 사용하려면 Vector를 사용하는게 좋지 않을까?
		PostProcess* m_postProcess;
		GraphicsPSO m_postProcessPSO;
		// TODO: 임시로 여기서 Tone Mapping 사용, Scene이나 AppBase에서 하는게 좋아보임
		std::shared_ptr<ToneMappingFilter> m_toneMapping;
		Texture2D m_toneMapTexture;
	};
}
