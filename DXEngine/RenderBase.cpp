#include "pch.h"
#include "RenderBase.h"
#include "GeometryGenerator.h"
#include "D3D11Utils.h"
#include "MeshData.h"
#include "PostProcess.h"
#include "ToneMappingFilter.h"

namespace DE {
	GraphicsCommon RenderBase::graphicsCommon;

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

		m_screenWidth = window.width;
		m_screenHeight = window.height;

		// Swap-Chain 설정
		DXGI_SWAP_CHAIN_DESC sd;
		ZeroMemory(&sd, sizeof(sd)); // 메모리 초기화
		sd.BufferDesc.Width = m_screenWidth;
		sd.BufferDesc.Height = m_screenHeight;
		sd.BufferDesc.Format = m_backBufferFormat;
		sd.BufferCount = 2; // double-buffering
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferUsage =  DXGI_USAGE_RENDER_TARGET_OUTPUT | // Rendering용
			// Compute Shader 용(CS에서 Back-Buffer를 사용할게 아니라면 필요없지만 후처리때 사용할 수 있으므로 설정)
			DXGI_USAGE_UNORDERED_ACCESS; 
		sd.OutputWindow = window.hwnd; // 렌더링할 윈도우
		sd.Windowed = TRUE; // windowed/full-screen
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // full-screen 모드 변경 가능
		//sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
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
		CreateBuffers();
		// Viewport 설정
		SetViewport();
		// DepthStencilView 생성
		CreateDepthStencilBuffer();

		graphicsCommon.InitCommonStates(m_device);

		// TODO: 임시
		D3D11Utils::CreateImageFilterTexture(m_device, int(m_screenViewport.Width), int(m_screenViewport.Height), m_toneMapTexture);
		m_toneMapping = std::make_shared<ToneMappingFilter>();
		m_toneMapping->Initialize(*this, { m_toneMapTexture.GetSRV() }, { m_backBufferRTV }, int(m_screenViewport.Width), int(m_screenViewport.Height));

		return true;
	}

	void RenderBase::Update()
	{
		if (m_postProcess)
			m_postProcess->Update(m_context);

		// TODO: 임시
		m_toneMapping->Update(m_context);
	}

	void RenderBase::Render()
	{
		m_context->RSSetViewports(1, &m_screenViewport);
	}

	void RenderBase::PostRender()
	{
		// 후처리 필터 시작하기 전에 Texture2DMS에 렌더링 된 결과를 Texture2D로 복사
		//ComPtr<ID3D11Texture2D> backBuffer;
		//ThrowIfFailed(m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf())));
		//m_context->CopyResource(m_tempTexture.Get(), m_floatBuffer.GetTexture());

		// Set PostProcessing GraphcisPSO
		SetPipelineState(m_postProcessPSO);
		if (m_postProcess)
			m_postProcess->Render(*this);

		// TODO: 임시
		m_toneMapping->Render(*this);

		// 현재 프레임 결과 복사
		ComPtr<ID3D11Texture2D> backBuffer;
		ThrowIfFailed(m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf())));
		m_context->CopyResource(m_prevFrame.GetTexture(), backBuffer.Get()); // 모션 블러 효과를 위해 렌더링 결과 보관
		//m_context->CopyResource(m_prevFrame.GetTexture(), m_floatBuffer.GetTexture()); // 모션 블러 효과를 위해 렌더링 결과 보관
	}

	void RenderBase::Present()
	{
		m_swapChain->Present(1, 0);
	}

	void RenderBase::SetRender()
	{
		float clearColor[4] = { 0.f, 0.f, 0.f, 1.f };
		m_context->ClearRenderTargetView(m_floatBuffer.GetRTV(), clearColor);
		m_context->ClearRenderTargetView(m_indexRTV.Get(), clearColor); // Mouse Picking
		m_context->ClearDepthStencilView(m_defaultDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

		// Multiple Render Targets
		ID3D11RenderTargetView* targets[] = { m_floatBuffer.GetRTV(), m_indexRTV.Get() };
		m_context->OMSetRenderTargets(2, targets, m_defaultDSV.Get());
	}

	void RenderBase::CreateBuffers()
	{
		// Raterization -> float/depthBuffer(MSAA) -> resolved -> backBuffer
		// 지금은 MSAA를 사용 안하니 Raterization -> float -> backBuffer 흐름 (HDR Pipeline)
		
		// BackBuffer는 화면으로 최종 출력 (SRV는 불필요)
		ComPtr<ID3D11Texture2D> backBuffer;
		ThrowIfFailed(m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));
		ThrowIfFailed(m_device->CreateRenderTargetView(backBuffer.Get(), NULL, m_backBufferRTV.GetAddressOf()));

		// FLOAT MSAA RenderTargetView/ShaderResourceView
		//ThrowIfFailed(m_device->CheckMultisampleQualityLevels(DXGI_FORMAT_R16G16B16A16_FLOAT, 4, &m_numQualityLevels));

		D3D11_TEXTURE2D_DESC desc;
		backBuffer->GetDesc(&desc);
		desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		desc.MipLevels = desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		desc.Usage = D3D11_USAGE_DEFAULT; // Staging Texture로부터 복사 가능
		desc.MiscFlags = 0;
		desc.CPUAccessFlags = 0;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		D3D11Utils::CreateTexture(m_device, desc, m_floatBuffer);

		// TODO: postEffect Buffer는 float buffer랑 똑같은 설정으로 생성
		//D3D11Utils::CreateTexture(m_device, desc, m_postEffectsBuffer);

		// Mouse Picking
		// 1x1 작은 Staging Texture 생성 (Pixel의 값을 GPU에서 CPU로 복사할 수 있도록 설정한 Texture)
		D3D11Utils::CreateStagingTexture(m_device, 1, 1, m_indexStagingTexture, m_backBufferFormat);

		// Mouse Picking에 사용할 Index 색을 렌더링할 Texture와 RenderTargetVeiw 생성
		backBuffer->GetDesc(&desc);
		ThrowIfFailed(m_device->CreateTexture2D(&desc, nullptr, m_indexTexture.GetAddressOf())); 
		ThrowIfFailed(m_device->CreateRenderTargetView(m_indexTexture.Get(), NULL, m_indexRTV.GetAddressOf()));
		
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		ThrowIfFailed(m_device->CreateTexture2D(&desc, nullptr, m_indexTempTexture.GetAddressOf())); // Index-Buffer 결과를 복사해서 임시 저장

		// 이전 프레임 저장용
		D3D11Utils::CreateTexture(m_device, backBuffer, m_prevFrame);
	}

	void RenderBase::SetViewport()
	{
		// Viewport 설정
		ZeroMemory(&m_screenViewport, sizeof(D3D11_VIEWPORT));
		m_screenViewport.TopLeftX = 0;
		m_screenViewport.TopLeftY = 0;
		m_screenViewport.Width = float(m_screenWidth);
		m_screenViewport.Height = float(m_screenHeight);
		m_screenViewport.MinDepth = 0.f;
		m_screenViewport.MaxDepth = 1.f;

		m_context->RSSetViewports(1, &m_screenViewport);
	}

	void RenderBase::CreateDepthStencilBuffer()
	{
		D3D11_TEXTURE2D_DESC dsDesc;
		dsDesc.Width = m_screenWidth;
		dsDesc.Height = m_screenHeight;
		dsDesc.MipLevels = 1; // Depth Stencil Buffer는 Mipmap 불필요
		dsDesc.ArraySize = 1;
		dsDesc.Usage = D3D11_USAGE_DEFAULT;
		dsDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		dsDesc.CPUAccessFlags = 0;
		dsDesc.MiscFlags = 0;
		dsDesc.SampleDesc.Count = 1;
		dsDesc.SampleDesc.Quality = 0;
		dsDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

		ComPtr<ID3D11Texture2D> depthStencilBuffer;
		ThrowIfFailed(m_device->CreateTexture2D(&dsDesc, 0, depthStencilBuffer.GetAddressOf()));
		ThrowIfFailed(m_device->CreateDepthStencilView(depthStencilBuffer.Get(), NULL, m_defaultDSV.GetAddressOf()));
	
		// Depth Only (Stencil이 필요가 없기에 Depth만 32bit 전부 사용)
		// Typeless로 선언해 DepthStencilView에서는 D32 Format을 사용, ShaderResourceView에서는 R32 Format을 사용
		// 두 Format이 서로 다르기 때문에 Typeless를 사용
		dsDesc.Format = DXGI_FORMAT_R32_TYPELESS; 
		dsDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		ThrowIfFailed(m_device->CreateTexture2D(&dsDesc, NULL, m_depthOnlyBuffer.GetAddressOfTexture()));

		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		ZeroMemory(&dsvDesc, sizeof(dsvDesc));
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT; // D32 Format 사용
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		ThrowIfFailed(m_device->CreateDepthStencilView(m_depthOnlyBuffer.GetTexture(), &dsvDesc, m_depthOnlyDSV.GetAddressOf()));

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT; // SRV로 사용하기 위해 R32 format 사용
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		ThrowIfFailed(m_device->CreateShaderResourceView(m_depthOnlyBuffer.GetTexture(), &srvDesc, m_depthOnlyBuffer.GetAddressOfSRV()));
		
		// Shadow Map용 Texture 생성
		dsDesc.Width = m_shadowWidth;
		dsDesc.Height = m_shadowHeight;
		for (int i = 0; i < MAX_LIGHTS; ++i) {
			ThrowIfFailed(m_device->CreateTexture2D(&dsDesc, NULL, m_shadowBuffers[i].GetAddressOfTexture()));
			// Shadow Map용 DSV 생성 (dsvDesc는 위에서 DepthOnlyDSV만들떄 사용한 걸 그대로 사용)
			ThrowIfFailed(m_device->CreateDepthStencilView(m_shadowBuffers[i].GetTexture(), &dsvDesc, m_shadowDSVs[i].GetAddressOf()));
			// Shadow Map용 SRV 생성 (위에서 만든 srvDesc 그대로 사용)
			ThrowIfFailed(m_device->CreateShaderResourceView(m_shadowBuffers[i].GetTexture(), &srvDesc, m_shadowBuffers[i].GetAddressOfSRV()));
		}
	}


	void RenderBase::SetDepthOnlyRender()
	{
		m_context->ClearDepthStencilView(m_depthOnlyDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
		// DepthOnly라서 RTV 불필요
		m_context->OMSetRenderTargets(0, NULL, m_depthOnlyDSV.Get());
	}

	void RenderBase::ResizeSwapChain(const WindowInfo& window)
	{
		m_screenWidth = window.width;
		m_screenHeight = window.height;

		m_backBufferRTV.Reset();
		// Swap Chain의 해상도를 변경하고 버퍼 개수를 유지/변경, Pixel Format 유지/변경, Flag 설정들을 해줄 수 있음
		m_swapChain->ResizeBuffers(
			0, // 현재 개수 유지
			// 해상도 변경
			UINT(m_screenWidth),
			UINT(m_screenHeight),
			DXGI_FORMAT_UNKNOWN, // 현재 포맷 유지
			0);
		// 해상도가 바뀌며 SwapChain을 다시 만들었기 때문에 다시 RTV와 DepthStencilBuffer 생성
		// 렌더링될 화면의 해상도가 바뀌면  Pixel의 개수 자체가 바뀌는 것이기 때문
		CreateBuffers();
		CreateDepthStencilBuffer();
		// 해상도에 맞는 Viewport 설정
		SetViewport();
	}

	void RenderBase::SetPipelineState(const GraphicsPSO& pso)
	{
		m_context->VSSetShader(pso.vertexShader.Get(), 0, 0);
		m_context->PSSetShader(pso.pixelShader.Get(), 0, 0);
		m_context->HSSetShader(pso.hullShader.Get(), 0, 0);
		m_context->DSSetShader(pso.domainShader.Get(), 0, 0);
		m_context->GSSetShader(pso.geometryShader.Get(), 0, 0);
		m_context->CSSetShader(NULL, 0, 0);
		m_context->IASetInputLayout(pso.inputLayout.Get());
		m_context->RSSetState(pso.rasterizerState.Get());
		m_context->OMSetBlendState(pso.blendState.Get(), pso.blendFactor, 0xffffffff); // 마지막 parameter는 multi-sample을 사용할때 사용
		m_context->OMSetDepthStencilState(pso.depthStencilState.Get(), pso.stencilRef);
		m_context->IASetPrimitiveTopology(pso.primitiveTopology);
	}

	void RenderBase::SetPostProcess(PostProcess& postProcess, const GraphicsPSO& pso)
	{
		postProcess.Initialize(*this, { m_floatBuffer.GetSRV(), m_prevFrame.GetSRV() }, { m_toneMapTexture.GetRTV() }, int(m_screenViewport.Width), int(m_screenViewport.Height));
		m_postProcess = &postProcess;
		m_postProcessPSO = pso;
	}

	void RenderBase::CopyIndexForPicking(int mouseX, int mouseY, uint8_t* dest)
	{
		// Mouse Picking Text (TODO)
		{
			m_context->CopyResource(m_indexTempTexture.Get(), m_indexTexture.Get());

			// 마우스 커서의 픽셀 한개만 복사 Staging Texture로 복사
			D3D11_BOX box;
			box.left = mouseX;
			box.right = mouseX + 1;
			box.top = mouseY;
			box.bottom = mouseY + 1;
			box.front = 0;
			box.back = 1;
			m_context->CopySubresourceRegion(m_indexStagingTexture.Get(), 0, 0, 0, 0,
				m_indexTempTexture.Get(), 0, &box);


			// GPU에서 CPU로 데이터 복사
			D3D11_MAPPED_SUBRESOURCE ms;
			m_context->Map(m_indexStagingTexture.Get(), NULL, D3D11_MAP_READ, NULL,
				&ms);
			// 픽셀 하나의 값만 복사
			memcpy(dest, ms.pData, sizeof(uint8_t) * 4);
			m_context->Unmap(m_indexStagingTexture.Get(), NULL);

			//D3D11Utils::CopyFromStagingTexture(m_context, m_indexStagingTexture, sizeof(uint8_t) * 4, dest);
		}
	}

	void RenderBase::ClearDepthBuffer()
	{
		m_context->ClearDepthStencilView(m_defaultDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	}

	void RenderBase::SetShadowViewport()
	{
		// Shadow Mapping에 맞는 Viewport를 설정
		D3D11_VIEWPORT shadowViewport;
		ZeroMemory(&shadowViewport, sizeof(shadowViewport));
		shadowViewport.TopLeftX = 0;
		shadowViewport.TopLeftY = 0;
		// Shadow Map으로 사용할 Depth Buffer의 해상도를 따로 설정해줬음
		shadowViewport.Width = float(m_shadowWidth);
		shadowViewport.Height = float(m_shadowHeight);
		shadowViewport.MinDepth = 0.f;
		shadowViewport.MaxDepth = 1.f;

		m_context->RSSetViewports(1, &shadowViewport);
	}

	void RenderBase::SetShadowMapRender(int idx)
	{
		// RTS 생략 가능
		m_context->OMSetRenderTargets(0, NULL, m_shadowDSVs[idx].Get());
		m_context->ClearDepthStencilView(m_shadowDSVs[idx].Get(),
			D3D11_CLEAR_DEPTH, 1.0f, 0);
	}

	void RenderBase::SetShadowSRVs()
	{
		std::vector<ID3D11ShaderResourceView*> shadowSRVs;
		for (int i = 0; i < MAX_LIGHTS; ++i)
			shadowSRVs.emplace_back(m_shadowBuffers[i].GetSRV());

		m_context->PSSetShaderResources(15, UINT(shadowSRVs.size()), shadowSRVs.data());
	}

	void RenderBase::ClearStencilBuffer()
	{
		// Stencil만 0으로 Clear(초기화)
		m_context->ClearDepthStencilView(m_defaultDSV.Get(), D3D11_CLEAR_STENCIL, 1.0f, 0);
	}
}