#pragma once
#include "D3D11Utils.h"
#include "GraphicsCommon.h"
#include "LightActor.h"
#include "ComputeCommon.h"
#include "Mesh.h"

namespace DE {
	class GraphicsCommon;
	class GraphicsPSO;
	class ComputePSO;
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
		// HDR Pipeline 
		void CreateBuffers();
		// Viewport 
		void SetViewport();
		// DepthStencilView Buffer 
		void CreateDepthStencilBuffer();
		void SetDepthOnlyRender();

		void CreateShadowArrayBuffer(const std::vector<LightActor*>& lights);

		ComPtr<ID3D11Device>& GetDevice() {	return m_device; }
		ComPtr<ID3D11DeviceContext>& GetContext() {	return m_context; }
		ComPtr<IDXGISwapChain>& GetSwapChain() { return m_swapChain; }
		void ResizeSwapChain(const WindowInfo& window);

		void SetPipelineState(const GraphicsPSO& pso);
		void SetPipelineState(const ComputePSO& pso);
		
		void SetPostProcess(PostProcess& postProcess, const GraphicsPSO& pso);

		void CopyIndexForPicking(int mouseX, int mouseY, uint8_t* dest);

		void ClearStencilBuffer();
		void ClearDepthBuffer();
		Texture2D& GetDepthOnlyBuffer() { return m_depthOnlyBuffer; };
		// Shadow Map viewport 
		void SetShadowViewport(float width, float height);
		void SetShadowSRVs();
		void SetShadowMap(int idx);

		// Particle
		void SetLowResRender();
		void DownsampleDepthToLowRes();

		void RenderCompositeLowResParticles();
		ID3D11ShaderResourceView* GetLowResSceneDepthSRV() const { return m_lowResDepthUAV.GetSRV(); }
		ID3D11ShaderResourceView* GetFullResSceneDepthSRV() const { return m_depthOnlyBuffer.GetSRV(); }

		static GraphicsCommon graphicsCommon;
		static ComputeCommon computeCommon;
	protected:
		UINT m_numQualityLevels = 0;
		int m_screenWidth = 1280;
		int m_screenHeight = 720;
		ComPtr<ID3D11Device> m_device;
		ComPtr<ID3D11DeviceContext> m_context;
		ComPtr<IDXGISwapChain> m_swapChain;
		ComPtr<ID3D11RenderTargetView> m_backBufferRTV;

		// ?? -> float(MSAA) -> Resolved(No MSAA) -> Post-Process -> BackBuffer( Swap-Chain Present)
		Texture2D m_floatBuffer;
		Texture2D m_resolvedBuffer;
		//Texture2D m_postEffectsBuffer;
		
		// Picking
		ComPtr<ID3D11Texture2D> m_indexTempTexture;
		ComPtr<ID3D11Texture2D> m_indexTexture; // Picking  Index  Texture
		ComPtr<ID3D11RenderTargetView> m_indexRTV; 
		ComPtr<ID3D11Texture2D> m_indexStagingTexture; // Picking 1x1 pixel data

		// TODO
		//ComPtr<ID3D11Texture2D> m_tempTexture;
		//ComPtr<ID3D11ShaderResourceView> m_backBufferSRV; // PostProcessing  SRV 
		Texture2D m_prevFrame;

		// Swap-Buffer Back Buffer
		DXGI_FORMAT m_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM; // 32-bit color (Low Dynamic Range Image)
		// [0.0, 1.0] float range
		//DXGI_FORMAT m_backBufferFormat = DXGI_FORMAT_R16G16B16A16_FLOAT; // 64-bit color (HDR Pipeline )

		D3D11_VIEWPORT m_screenViewport = D3D11_VIEWPORT();
		bool m_drawAsWire = false;

		// Depth-Stencil Buffer
		ComPtr<ID3D11DepthStencilView> m_defaultDSV;

		Texture2D m_depthOnlyBuffer;
		ComPtr<ID3D11DepthStencilView> m_depthOnlyDSV;

		// TODO: PostProcess Vector로 여러 PostProcessing 적용
		PostProcess* m_postProcess = nullptr;
		GraphicsPSO m_postProcessPSO;
		// TODO: ToneMapping을 AppBase에서 마지막에 실행하는건?
		std::shared_ptr<ToneMappingFilter> m_toneMapping;
		Texture2D m_toneMapTexture;

		// Shadow Map은 Texture를 1:1 Ratio를 사용
		int m_shadowWidth = 1280;
		int m_shadowHeight = 1280; 
		Texture2D m_shadowArrayBuffer; // light의 Shadow
		std::vector<ComPtr<ID3D11DepthStencilView>> m_shadowDSVs;

		// Particle 
		ComPtr<ID3D11DepthStencilView> m_lowResDSV;
		Texture2D m_lowResDepth;
		Texture2D m_lowResDepthUAV;  // Temporary UAV texture for depth downsampling

		int m_lowResWidth = static_cast<int>(m_screenWidth * 0.5f);
		int m_lowResHeight = static_cast<int>(m_screenHeight * 0.5f);
		Texture2D m_lowResParticleTexture;

		D3D11_VIEWPORT m_lowResViewport = D3D11_VIEWPORT();
		std::shared_ptr<Mesh> m_compositeQuad;

		// Depth downsampling
		struct DepthDownsampleConsts {
			UINT outputWidth;
			UINT outputHeight;
			UINT padding[2];
		};
		ComPtr<ID3D11Buffer> m_depthDownsampleCB;
	};
}
