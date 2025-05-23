#include "pch.h"
#include "RenderBase.h"


#include "GeometryGenerator.h"
#include "D3D11Utils.h"
#include "MeshData.h"

namespace DE {
	RenderBase::RenderBase() : m_screenViewport(D3D11_VIEWPORT())
	{
	}
	RenderBase::~RenderBase()
	{
	}

	bool RenderBase::Initialize(WindowInfo& window)
	{
		// 그래픽카드 하드ㅜ에어 호환성 설정
		const D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_HARDWARE;

		UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG; // 그래픽스 디버깅 활성화
#endif

		// DirectX 버전 (나중에 추가 가능) - 더 높은 버전이 먼저 오도록 설정
		const D3D_FEATURE_LEVEL featureLevels[1] = {
			D3D_FEATURE_LEVEL_11_0
		};
		D3D_FEATURE_LEVEL featureLevel;

		// Swap-Chain 설정
		DXGI_SWAP_CHAIN_DESC sd;
		ZeroMemory(&sd, sizeof(sd)); // 메모리 초기화
		sd.BufferDesc.Width = window.width;
		sd.BufferDesc.Height = window.height;
		sd.BufferDesc.Format = m_backBufferFormat;
		sd.BufferCount = 2; // double-buffering
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | // Rendering용
			// Compute Shader 용(CS에서 Back-Buffer를 사용할게 아니라면 필요없지만 후처리때 사용할 수 있으므로 설정)
			DXGI_USAGE_UNORDERED_ACCESS; 
		sd.OutputWindow = window.hwnd; // 렌더링할 윈도우
		sd.Windowed = TRUE; // windowed/full-screen
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // full-screen 모드 변경 가능
		sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		// No MSAA
		sd.SampleDesc.Count = 1; 
		sd.SampleDesc.Quality = 0;

		// Device, Device Context, SwapChain 생성
		ThrowIfFailed(::D3D11CreateDeviceAndSwapChain(
			0, driverType, 0, createDeviceFlags, featureLevels, ARRAYSIZE(featureLevels),
			D3D11_SDK_VERSION, &sd, m_swapChain.GetAddressOf(),
			m_device.GetAddressOf(), &featureLevel, m_context.GetAddressOf()));

		//window.device = m_device;
		//window.context = m_context;

		// 원하는 D3D 버전인지 확인
		if (featureLevel != D3D_FEATURE_LEVEL_11_0) {
			std::cout << "D3D Feature Level 11 unsupported." << std::endl;
			return false;
		}

		// Back Buffer의 RTV 생성
		CreateBackBufferRTV();
		// Viewport 설정
		SetViewport(window);
		// RasterizationState 생성
		InitRS();
		// DepthStencilState 설정
		InitDepthStencilState();
		// DepthStencilView 생성
		CreateDepthStencilBuffer(window);

		return true;
	}

	void RenderBase::Update()
	{
		
	}

	void RenderBase::Render()
	{
		m_context->RSSetViewports(1, &m_screenViewport);

		float clearColor[4] = { 0.f, 0.f, 0.f, 1.f };
		m_context->ClearRenderTargetView(m_backBufferRTV.Get(), clearColor);
		m_context->ClearDepthStencilView(m_defaultDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

		m_context->OMSetRenderTargets(1, m_backBufferRTV.GetAddressOf(), m_defaultDSV.Get());
		m_context->OMSetDepthStencilState(m_defaultDSS.Get(), 0);

		m_context->RSSetState(m_solidRS.Get());
	}

	void RenderBase::Present()
	{
		m_swapChain->Present(1, 0);
	}

	void RenderBase::CreateBackBufferRTV()
	{
		// Raterization -> float/depthBuffer(MSAA) -> resolved -> backBuffer
		
		// BackBuffer는 화면으로 최종 출력 (SRV는 불필요)
		ComPtr<ID3D11Texture2D> backBuffer;
		ThrowIfFailed(m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));
		ThrowIfFailed(m_device->CreateRenderTargetView(backBuffer.Get(), NULL, m_backBufferRTV.GetAddressOf()));
	}

	void RenderBase::SetViewport(const WindowInfo& window)
	{
		// Viewport 설정
		ZeroMemory(&m_screenViewport, sizeof(D3D11_VIEWPORT));
		m_screenViewport.TopLeftX = 0;
		m_screenViewport.TopLeftY = 0;
		m_screenViewport.Width = float(window.width);
		m_screenViewport.Height = float(window.height);
		m_screenViewport.MinDepth = 0.f;
		m_screenViewport.MaxDepth = 1.f;

		m_context->RSSetViewports(1, &m_screenViewport);
	}
	void RenderBase::SetViewport(const float& width, const float& height)
	{		
		// Viewport 설정
		ZeroMemory(&m_screenViewport, sizeof(D3D11_VIEWPORT));
		m_screenViewport.TopLeftX = 0;
		m_screenViewport.TopLeftY = 0;
		m_screenViewport.Width = width;
		m_screenViewport.Height = height;
		m_screenViewport.MinDepth = 0.f;
		m_screenViewport.MaxDepth = 1.f;

		m_context->RSSetViewports(1, &m_screenViewport);
	}
	void RenderBase::InitRS()
	{
		D3D11_RASTERIZER_DESC rastDesc;
		ZeroMemory(&rastDesc, sizeof(D3D11_RASTERIZER_DESC));
		rastDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
		rastDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_BACK;
		rastDesc.FrontCounterClockwise = false;
		rastDesc.DepthClipEnable = true; // zNear, zFar
		rastDesc.MultisampleEnable = true;

		ThrowIfFailed(m_device->CreateRasterizerState(&rastDesc, m_solidRS.GetAddressOf()));
	}

	void RenderBase::InitDepthStencilState()
	{
		// Default
		D3D11_DEPTH_STENCIL_DESC dsDesc;
		ZeroMemory(&dsDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
		dsDesc.DepthEnable = true;
		// 경우에 따라 Depth Buffer를 껏다 켰다할 때 사용할 수 있음
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL;
		// Depth값이 더 작은걸 렌더링
		dsDesc.DepthFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS;
		// 기본 DS에선 Stencil 불필요
		dsDesc.StencilEnable = false;
		ThrowIfFailed(m_device->CreateDepthStencilState(&dsDesc, m_defaultDSS.GetAddressOf()));
	}

	void RenderBase::CreateDepthStencilBuffer(const WindowInfo& window)
	{
		D3D11_TEXTURE2D_DESC dsBufferDesc;
		dsBufferDesc.Width = window.width;
		dsBufferDesc.Height = window.height;
		dsBufferDesc.MipLevels = 1;
		dsBufferDesc.ArraySize = 1;
		dsBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		dsBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		dsBufferDesc.CPUAccessFlags = 0;
		dsBufferDesc.MiscFlags = 0;
		dsBufferDesc.SampleDesc.Count = 1;
		dsBufferDesc.SampleDesc.Quality = 0;
		dsBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

		ComPtr<ID3D11Texture2D> depthStencilBuffer;
		ThrowIfFailed(m_device->CreateTexture2D(&dsBufferDesc, 0, depthStencilBuffer.GetAddressOf()));
		ThrowIfFailed(m_device->CreateDepthStencilView(depthStencilBuffer.Get(), NULL, m_defaultDSV.GetAddressOf()));
	}

	void RenderBase::ResizeSwapChain(const WindowInfo& window)
	{
		m_backBufferRTV.Reset();
		// Swap Chain의 해상도를 변경하고 버퍼 개수를 유지/변경, Pixel Format 유지/변경, Flag 설정들을 해줄 수 있음
		m_swapChain->ResizeBuffers(
			0, // 현재 개수 유지
			// 해상도 변경
			UINT(window.width),
			UINT(window.height),
			DXGI_FORMAT_UNKNOWN, // 현재 포맷 유지
			0);
		// 해상도가 바뀌며 SwapChain을 다시 만들었기 때문에 다시 RTV와 DepthStencilBuffer 생성
		// 렌더링될 화면의 해상도가 바뀌면  Pixel의 개수 자체가 바뀌는 것이기 때문
		CreateBackBufferRTV();
		CreateDepthStencilBuffer(window);
		// 해상도에 맞는 Viewport 설정
		SetViewport(window);
	}
}