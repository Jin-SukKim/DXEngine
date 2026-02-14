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
		// HDR Pipeline�� �ʿ��� Buffer�� ����
		void CreateBuffers();
		// �������ϰ� ���� ȭ�� ũ�⿡ �´� Viewport ����
		void SetViewport();
		// DepthStencilView Buffer ����
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

		// Stencil Buffer�� 0���� �ʱ�ȭ
		void ClearStencilBuffer();
		// Depth Buffer�� 1.0���� �ʱ�ȭ
		void ClearDepthBuffer();
		Texture2D& GetDepthOnlyBuffer() { return m_depthOnlyBuffer; };
		// Shadow Map�� viewport ����
		void SetShadowViewport(float width, float height);
		// �׸��ڸʵ鵵 ���� Texture�� ���Ŀ� �߰��ϰ� �ִ� ������ PS�� t15���� �߰�
		void SetShadowSRVs();
		void SetShadowMap(int idx);

		// Particle
		void SetLowResRender();
		void DownsampleDepthToLowRes();

		void RenderCompositeLowResParticles();

		// �̸� �����ص� Setting��
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

		// �ﰢ�� ������ȭ -> float(MSAA) -> Resolved(No MSAA) -> Post-Process -> BackBuffer(���� Swap-Chain Present)
		Texture2D m_floatBuffer;
		Texture2D m_resolvedBuffer;
		//Texture2D m_postEffectsBuffer;
		
		// Picking
		ComPtr<ID3D11Texture2D> m_indexTempTexture;
		ComPtr<ID3D11Texture2D> m_indexTexture; // Picking�� ���� Index�� ������ Texture
		ComPtr<ID3D11RenderTargetView> m_indexRTV; 
		ComPtr<ID3D11Texture2D> m_indexStagingTexture; // Picking�� �ϸ� ������ 1x1 pixel data

		// TODO
		//ComPtr<ID3D11Texture2D> m_tempTexture;
		//ComPtr<ID3D11ShaderResourceView> m_backBufferSRV; // �ӽ÷� PostProcessing�� ���� SRV ����
		Texture2D m_prevFrame;

		// Swap-Buffer�� Back Buffer ������ �����ؼ� ����� �� ����
		DXGI_FORMAT m_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM; // 32-bit color (Low Dynamic Range Image)
		// [0.0, 1.0]���� ������ ������ �ƴ� float���� �� ���� ������ ���ؼ� �������� �� �� ����
		//DXGI_FORMAT m_backBufferFormat = DXGI_FORMAT_R16G16B16A16_FLOAT; // 64-bit color (HDR Pipeline ���)

		D3D11_VIEWPORT m_screenViewport = D3D11_VIEWPORT();
		bool m_drawAsWire = false;

		// Depth-Stencil Buffer
		ComPtr<ID3D11DepthStencilView> m_defaultDSV;

		Texture2D m_depthOnlyBuffer;
		ComPtr<ID3D11DepthStencilView> m_depthOnlyDSV;

		// TODO: ���� ���� PostProcess�� ����Ϸ��� Vector�� ����ϴ°� ���� ������?
		PostProcess* m_postProcess = nullptr;
		GraphicsPSO m_postProcessPSO;
		// TODO: �ӽ÷� ���⼭ Tone Mapping ���, Scene�̳� AppBase���� �ϴ°� ���ƺ���
		std::shared_ptr<ToneMappingFilter> m_toneMapping;
		Texture2D m_toneMapTexture;

		// Shadow
		// Shadow Map�� �ػ󵵰� �ٸ��� ȭ�� �ػ󵵿� ���� �ʿ䰡 ����
		// ���� Texture�� ���簢���̱� ������ ratio�� 1:1�� �ػ󵵷� ����
		int m_shadowWidth = 1280;
		int m_shadowHeight = 1280; 
		// ������ �׸��� �� �ػ󵵿� ���缭 �׸��ڸʿ� viewport ����
		Texture2D m_shadowArrayBuffer; // light ������ŭ shadow �� ����
		std::vector<ComPtr<ID3D11DepthStencilView>> m_shadowDSVs;

		// Particle ��
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
