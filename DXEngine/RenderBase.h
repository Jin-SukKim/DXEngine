#pragma once
#include "D3D11Utils.h"
#include "GraphicsCommon.h"
#include "LightActor.h"

namespace DE {
	class GraphicsCommon;
	class GraphicsPSO;
	class PostProcess;
	class ToneMappingFilter;

	class RenderBase
	{
		GENERATE_SINGLE(RenderBase)
	public:
		virtual ~RenderBase();

		virtual bool Initialize(WindowInfo& window);
		virtual void Update();
		virtual void Render();
		virtual void PostRender();
		void Present();

		void SetRender();
		// HDR Pipeline에 필요한 Buffer들 생성
		void CreateBuffers();
		// 렌더링하고 싶은 화면 크기에 맞는 Viewport 설정
		void SetViewport();
		// DepthStencilView Buffer 생성
		void CreateDepthStencilBuffer();
		void SetDepthOnlyRender();

		void CreateShadowArrayBuffer(const std::vector<std::shared_ptr<Actor>>& lights);

		ComPtr<ID3D11Device>& GetDevice() {	return m_device; }
		ComPtr<ID3D11DeviceContext>& GetContext() {	return m_context; }
		ComPtr<IDXGISwapChain>& GetSwapChain() { return m_swapChain; }
		void ResizeSwapChain(const WindowInfo& window);

		void SetPipelineState(const GraphicsPSO& pso);
		
		void SetPostProcess(PostProcess& postProcess, const GraphicsPSO& pso);

		void CopyIndexForPicking(int mouseX, int mouseY, uint8_t* dest);

		// Stencil Buffer만 0으로 초기화
		void ClearStencilBuffer();
		// Depth Buffer만 1.0으로 초기화
		void ClearDepthBuffer();
		Texture2D& GetDepthOnlyBuffer() { return m_depthOnlyBuffer; };
		// Shadow Map용 viewport 설정
		void SetShadowViewport(float width, float height);
		// 그림자맵들도 공요 Texture들 이후에 추가하고 있는 중으로 PS의 t15부터 추가
		void SetShadowSRVs();
		void SetShadowMap(int idx);

		// 미리 설정해둔 Setting들
		static GraphicsCommon graphicsCommon;
	protected:
		UINT m_numQualityLevels = 0;
		int m_screenWidth = 1280;
		int m_screenHeight = 720;
		ComPtr<ID3D11Device> m_device;
		ComPtr<ID3D11DeviceContext> m_context;
		ComPtr<IDXGISwapChain> m_swapChain;
		ComPtr<ID3D11RenderTargetView> m_backBufferRTV;

		// 삼각형 레스터화 -> float(MSAA) -> Resolved(No MSAA) -> Post-Process -> BackBuffer(최종 Swap-Chain Present)
		Texture2D m_floatBuffer;
		Texture2D m_resolvedBuffer;
		//Texture2D m_postEffectsBuffer;
		
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

		D3D11_VIEWPORT m_screenViewport = D3D11_VIEWPORT();
		bool m_drawAsWire = false;

		// Depth-Stencil Buffer
		ComPtr<ID3D11DepthStencilView> m_defaultDSV;

		Texture2D m_depthOnlyBuffer;
		ComPtr<ID3D11DepthStencilView> m_depthOnlyDSV;

		// TODO: 여러 개의 PostProcess를 사용하려면 Vector를 사용하는게 좋지 않을까?
		PostProcess* m_postProcess = nullptr;
		GraphicsPSO m_postProcessPSO;
		// TODO: 임시로 여기서 Tone Mapping 사용, Scene이나 AppBase에서 하는게 좋아보임
		std::shared_ptr<ToneMappingFilter> m_toneMapping;
		Texture2D m_toneMapTexture;

		// Shadow
		// Shadow Map은 해상도가 다른데 화면 해상도와 같을 필요가 없음
		// 보통 Texture가 정사각형이기 때문에 ratio가 1:1인 해상도로 설정
		int m_shadowWidth = 1280;
		int m_shadowHeight = 1280; 
		// 설정한 그림자 맵 해상도에 맞춰서 그림자맵용 viewport 설정
		Texture2D m_shadowArrayBuffer; // light 개수만큼 shadow 맵 생성
		std::vector<ComPtr<ID3D11DepthStencilView>> m_shadowDSVs;
	};
}
