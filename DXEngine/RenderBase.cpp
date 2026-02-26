#include "pch.h"
#include "RenderBase.h"
#include "GeometryGenerator.h"
#include "D3D11Utils.h"
#include "MeshData.h"
#include "PostProcess.h"
#include "ToneMappingFilter.h"
#include "ComputePSO.h"

namespace DE {
	GraphicsCommon RenderBase::graphicsCommon;
	ComputeCommon RenderBase::computeCommon;  // ߰

	RenderBase::~RenderBase()
	{
	}

	bool RenderBase::Initialize(WindowInfo& window)
	{
		// ׷ī ϵ̿ ȣȯ 
		const D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_HARDWARE;

		UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG; // ׷Ƚ  Ȱȭ
#endif

		// DirectX  (߿ ߰ ) -      
		const D3D_FEATURE_LEVEL featureLevels[1] = {
			D3D_FEATURE_LEVEL_11_0
		};
		D3D_FEATURE_LEVEL featureLevel;

		m_screenWidth = window.width;
		m_screenHeight = window.height;

		// Swap-Chain 
		DXGI_SWAP_CHAIN_DESC sd;
		ZeroMemory(&sd, sizeof(sd)); // ޸ ʱȭ
		sd.BufferDesc.Width = m_screenWidth;
		sd.BufferDesc.Height = m_screenHeight;
		sd.BufferDesc.Format = m_backBufferFormat;
		sd.BufferCount = 2; // double-buffering
		sd.BufferDesc.RefreshRate.Numerator = 0;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferUsage =  DXGI_USAGE_RENDER_TARGET_OUTPUT | // Rendering
			// Compute Shader (CS Back-Buffer Ұ ƴ϶ ʿ ó   Ƿ )
			DXGI_USAGE_UNORDERED_ACCESS; 
		sd.OutputWindow = window.hwnd; //  
		sd.Windowed = TRUE; // windowed/full-screen
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // full-screen   
		//sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		// No MSAA
		sd.SampleDesc.Count = 1; 
		sd.SampleDesc.Quality = 0;

		// Device, Device Context, SwapChain 
		ThrowIfFailed(::D3D11CreateDeviceAndSwapChain(
			0, driverType, 0, createDeviceFlags, featureLevels, ARRAYSIZE(featureLevels),
			D3D11_SDK_VERSION, &sd, m_swapChain.GetAddressOf(),
			m_device.GetAddressOf(), &featureLevel, m_context.GetAddressOf()));

		//window.device = m_device;
		//window.context = m_context;

		// ϴ D3D  Ȯ
		if (featureLevel != D3D_FEATURE_LEVEL_11_0) {
			std::cout << "D3D Feature Level 11 unsupported." << std::endl;
			return false;
		}

		// Back Buffer RTV 
		CreateBuffers();
		// Viewport 
		SetViewport();
		// DepthStencilView 
		CreateDepthStencilBuffer();

		graphicsCommon.InitCommonStates(m_device);
		computeCommon.InitCommonStates(m_device);  // ߰

		// Particle
		{
			// ȭ   簢   簢 Texture Shadingؼ(PostProcessing)  ȭ 
			MeshData quadData = GeometryGenerator::MakeSquare();
			m_compositeQuad = std::make_shared<Mesh>();
			D3D11Utils::CreateVertexBuffer(m_device, quadData.vertices, m_compositeQuad->vertexBuffer);
			m_compositeQuad->indexCount = UINT(quadData.indices.size());
			D3D11Utils::CreateIndexBuffer(m_device, quadData.indices, m_compositeQuad->indexBuffer);
			m_compositeQuad->stride = UINT(sizeof(Vertex));
			m_compositeQuad->offset = 0;
		}

		// TODO: ӽ
		D3D11Utils::CreateImageFilterTexture(m_device, int(m_screenViewport.Width), int(m_screenViewport.Height), m_toneMapTexture);
		m_toneMapping = std::make_shared<ToneMappingFilter>();
		m_toneMapping->Initialize({ m_toneMapTexture.GetSRV() }, { m_backBufferRTV }, int(m_screenViewport.Width), int(m_screenViewport.Height));

		return true;
	}

	void RenderBase::Update()
	{
		if (m_postProcess)
			m_postProcess->Update();

		// TODO: ӽ
		m_toneMapping->Update();
	}

	void RenderBase::Render()
	{
		m_context->RSSetViewports(1, &m_screenViewport);
	}

	void RenderBase::PostRender()
	{
		// ó  ϱ  Texture2DMS    Texture2D 
		//ComPtr<ID3D11Texture2D> backBuffer;
		//ThrowIfFailed(m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf())));
		//m_context->CopyResource(m_tempTexture.Get(), m_floatBuffer.GetTexture());

		// Set PostProcessing GraphcisPSO
		SetPipelineState(m_postProcessPSO);
		if (m_postProcess)
			m_postProcess->Render();

		// TODO: ӽ
		m_toneMapping->Render();

		//    
		ComPtr<ID3D11Texture2D> backBuffer;
		ThrowIfFailed(m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf())));
		m_context->CopyResource(m_prevFrame.GetTexture(), backBuffer.Get()); //   ȿ    
		//m_context->CopyResource(m_prevFrame.GetTexture(), m_floatBuffer.GetTexture()); //   ȿ    
	}

	void RenderBase::Present()
	{
		m_swapChain->Present(0, 0);
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
		//  MSAA  ϴ Raterization -> float -> backBuffer 帧 (HDR Pipeline)
		
		// BackBuffer ȭ   (SRV ʿ)
		ComPtr<ID3D11Texture2D> backBuffer;
		ThrowIfFailed(m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));
		ThrowIfFailed(m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, m_backBufferRTV.GetAddressOf()));

		// FLOAT MSAA RenderTargetView/ShaderResourceView
		//ThrowIfFailed(m_device->CheckMultisampleQualityLevels(DXGI_FORMAT_R16G16B16A16_FLOAT, 4, &m_numQualityLevels));

		D3D11_TEXTURE2D_DESC desc;
		backBuffer->GetDesc(&desc);
		desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		desc.MipLevels = desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		desc.Usage = D3D11_USAGE_DEFAULT; // Staging Textureκ  
		desc.MiscFlags = 0;
		desc.CPUAccessFlags = 0;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		D3D11Utils::CreateTexture(m_device, desc, m_floatBuffer);

		// TODO: postEffect Buffer float buffer Ȱ  
		//D3D11Utils::CreateTexture(m_device, desc, m_postEffectsBuffer);

		// Mouse Picking
		// 1x1  Staging Texture  (Pixel  GPU CPU   ֵ  Texture)
		D3D11Utils::CreateStagingTexture(m_device, 1, 1, m_indexStagingTexture, m_backBufferFormat);

		// Mouse Picking  Index   Texture RenderTargetVeiw 
		backBuffer->GetDesc(&desc);
		ThrowIfFailed(m_device->CreateTexture2D(&desc, nullptr, m_indexTexture.GetAddressOf())); 
		ThrowIfFailed(m_device->CreateRenderTargetView(m_indexTexture.Get(), nullptr, m_indexRTV.GetAddressOf()));
		
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		ThrowIfFailed(m_device->CreateTexture2D(&desc, nullptr, m_indexTempTexture.GetAddressOf())); // Index-Buffer  ؼ ӽ 

		//   
		D3D11Utils::CreateTexture(m_device, backBuffer, m_prevFrame);

		// Particle Low Res 
		m_lowResWidth = static_cast<int>(desc.Width * 0.5f);
		m_lowResHeight = static_cast<int>(desc.Height * 0.5f);
		desc.Width = m_lowResWidth;
		desc.Height = m_lowResHeight;
		desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		D3D11Utils::CreateTexture(m_device, desc, m_lowResParticleTexture);
	}

	void RenderBase::SetViewport()
	{
		// Viewport 
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
		dsDesc.MipLevels = 1; // Depth Stencil Buffer Mipmap ʿ
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
		ThrowIfFailed(m_device->CreateDepthStencilView(depthStencilBuffer.Get(), nullptr, m_defaultDSV.GetAddressOf()));
	
		// Depth Only (Stencil ʿ䰡 ⿡ Depth 32bit  )
		// Typeless  DepthStencilView D32 Format , ShaderResourceView R32 Format 
		//  Format  ٸ  Typeless 
		dsDesc.Format = DXGI_FORMAT_R32_TYPELESS; 
		dsDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		ThrowIfFailed(m_device->CreateTexture2D(&dsDesc, nullptr, m_depthOnlyBuffer.GetAddressOfTexture()));

		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		ZeroMemory(&dsvDesc, sizeof(dsvDesc));
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT; // D32 Format 
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		ThrowIfFailed(m_device->CreateDepthStencilView(m_depthOnlyBuffer.GetTexture(), &dsvDesc, m_depthOnlyDSV.GetAddressOf()));

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT; // SRV ϱ  R32 format 
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		ThrowIfFailed(m_device->CreateShaderResourceView(m_depthOnlyBuffer.GetTexture(), &srvDesc, m_depthOnlyBuffer.GetAddressOfSRV()));

		// Particle Overdraw 
		dsDesc.Width = m_lowResWidth;
		dsDesc.Height = m_lowResHeight;
		ThrowIfFailed(m_device->CreateTexture2D(&dsDesc, nullptr, m_lowResDepth.GetAddressOfTexture()));
		ThrowIfFailed(m_device->CreateDepthStencilView(m_lowResDepth.GetTexture(), &dsvDesc, m_lowResDSV.GetAddressOf()));
		ThrowIfFailed(m_device->CreateShaderResourceView(m_lowResDepth.GetTexture(), &srvDesc, m_lowResDepth.GetAddressOfSRV()));

		// Create separate UAV texture for depth downsampling (R32_FLOAT format)
		D3D11_TEXTURE2D_DESC uavTexDesc;
		ZeroMemory(&uavTexDesc, sizeof(uavTexDesc));
		uavTexDesc.Width = m_lowResWidth;
		uavTexDesc.Height = m_lowResHeight;
		uavTexDesc.MipLevels = 1;
		uavTexDesc.ArraySize = 1;
		uavTexDesc.Format = DXGI_FORMAT_R32_FLOAT;
		uavTexDesc.SampleDesc.Count = 1;
		uavTexDesc.SampleDesc.Quality = 0;
		uavTexDesc.Usage = D3D11_USAGE_DEFAULT;
		uavTexDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
		uavTexDesc.CPUAccessFlags = 0;
		uavTexDesc.MiscFlags = 0;
		ThrowIfFailed(m_device->CreateTexture2D(&uavTexDesc, nullptr, m_lowResDepthUAV.GetAddressOfTexture()));

		// Create UAV for the temporary texture
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		ZeroMemory(&uavDesc, sizeof(uavDesc));
		uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;
		ThrowIfFailed(m_device->CreateUnorderedAccessView(
			m_lowResDepthUAV.GetTexture(),
			&uavDesc,
			m_lowResDepthUAV.GetAddressOfUAV()));

		// Create SRV for the temporary texture (optional, for debugging)
		D3D11_SHADER_RESOURCE_VIEW_DESC uavSrvDesc;
		ZeroMemory(&uavSrvDesc, sizeof(uavSrvDesc));
		uavSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		uavSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		uavSrvDesc.Texture2D.MipLevels = 1;
		ThrowIfFailed(m_device->CreateShaderResourceView(
			m_lowResDepthUAV.GetTexture(),
			&uavSrvDesc,
			m_lowResDepthUAV.GetAddressOfSRV()));

		// Create constant buffer for depth downsampling
		DepthDownsampleConsts dsConsts = {
			static_cast<UINT>(m_lowResWidth),
			static_cast<UINT>(m_lowResHeight),
			0, 0
		};
		D3D11Utils::CreateConstantBuffer(m_device.Get(), dsConsts, m_depthDownsampleCB);
	}


	void RenderBase::SetDepthOnlyRender()
	{
		m_context->ClearDepthStencilView(m_depthOnlyDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
		// DepthOnly RTV ʿ
		m_context->OMSetRenderTargets(0, nullptr, m_depthOnlyDSV.Get());
	}

	void RenderBase::CreateShadowArrayBuffer(const std::vector<LightActor*>& lights)
	{
		if (lights[0] == nullptr)
			return;

		int arraySize = 0;
		LightActor* light;
		for (const auto& actor : lights) {
			if (actor != nullptr) {
				light = actor;
				if (light->GetLight().type & (LIGHT_SPOT | LIGHT_DIRECTIONAL))
					++arraySize;
				else if (light->GetLight().type & LIGHT_POINT)
					arraySize += 6;
			}
		}

		light = lights[0];
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Width = light->GetShadowWidth();
		desc.Height = light->GetShadowHeight();
		desc.MipLevels = 1; // Mipmap Level ִ
		desc.ArraySize = arraySize; // Texture Array̹Ƿ  Texture 
		desc.Format = DXGI_FORMAT_R32_TYPELESS;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT; // Staging Textureκ  
		desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

		// ʱ   Texture 
		ThrowIfFailed(m_device->CreateTexture2D(&desc, nullptr, m_shadowArrayBuffer.GetAddressOfTexture()));

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT; // SRV ϱ  R32 format 
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.MipLevels = 1;
		srvDesc.Texture2DArray.ArraySize = arraySize;
		// Shadow Map SRV  (  srvDesc ״ )
		ThrowIfFailed(m_device->CreateShaderResourceView(m_shadowArrayBuffer.GetTexture(), &srvDesc, m_shadowArrayBuffer.GetAddressOfSRV()));
	
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		ZeroMemory(&dsvDesc, sizeof(dsvDesc));
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT; // D32 Format 
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
		dsvDesc.Texture2DArray.MipSlice = 0;
		dsvDesc.Texture2DArray.ArraySize = 1; //  DSV ϳ 

		m_shadowDSVs.resize(arraySize);
		for (int i = 0; i < arraySize; ++i) {
			dsvDesc.Texture2DArray.FirstArraySlice = i;
			ThrowIfFailed(m_device->CreateDepthStencilView(m_shadowArrayBuffer.GetTexture(), &dsvDesc, m_shadowDSVs[i].GetAddressOf()));
		}
	}

	void RenderBase::ResizeSwapChain(const WindowInfo& window)
	{
		m_screenWidth = window.width;
		m_screenHeight = window.height;

		m_backBufferRTV.Reset();
		// 이전 tone map 리소스 해제 후 재생성
		m_toneMapTexture = Texture2D();

		m_swapChain->ResizeBuffers(
			0,
			UINT(m_screenWidth),
			UINT(m_screenHeight),
			DXGI_FORMAT_UNKNOWN,
			0);

		CreateBuffers();
		CreateDepthStencilBuffer();
		SetViewport();

		// Tone map 텍스처 및 필터 재생성
		D3D11Utils::CreateImageFilterTexture(m_device, m_screenWidth, m_screenHeight, m_toneMapTexture);
		m_toneMapping->Initialize({ m_toneMapTexture.GetSRV() }, { m_backBufferRTV }, m_screenWidth, m_screenHeight);

		// PostProcess도 새 해상도로 재초기화
		if (m_postProcess) {
			m_postProcess->Initialize({ m_floatBuffer.GetSRV(), m_prevFrame.GetSRV() },
				{ m_toneMapTexture.GetRTV() }, m_screenWidth, m_screenHeight);
		}
	}

	void RenderBase::SetPipelineState(const GraphicsPSO& pso)
	{
		m_context->VSSetShader(pso.vertexShader.Get(), 0, 0);
		m_context->PSSetShader(pso.pixelShader.Get(), 0, 0);
		m_context->HSSetShader(pso.hullShader.Get(), 0, 0);
		m_context->DSSetShader(pso.domainShader.Get(), 0, 0);
		m_context->GSSetShader(pso.geometryShader.Get(), 0, 0);
		m_context->CSSetShader(nullptr, 0, 0);
		m_context->IASetInputLayout(pso.inputLayout.Get());
		m_context->RSSetState(pso.rasterizerState.Get());
		m_context->OMSetBlendState(pso.blendState.Get(), pso.blendFactor, 0xffffffff); //  parameter multi-sample Ҷ 
		m_context->OMSetDepthStencilState(pso.depthStencilState.Get(), pso.stencilRef);
		m_context->IASetPrimitiveTopology(pso.primitiveTopology);
	}

	void RenderBase::SetPipelineState(const ComputePSO& pso)
	{
		m_context->VSSetShader(nullptr, 0, 0);
		m_context->PSSetShader(nullptr, 0, 0);
		m_context->HSSetShader(nullptr, 0, 0);
		m_context->DSSetShader(nullptr, 0, 0);
		m_context->GSSetShader(nullptr, 0, 0);
		m_context->CSSetShader(pso.computeShader.Get(), 0, 0);
	}

	void RenderBase::SetPostProcess(PostProcess& postProcess, const GraphicsPSO& pso)
	{
		postProcess.Initialize({ m_floatBuffer.GetSRV(), m_prevFrame.GetSRV() }, { m_toneMapTexture.GetRTV() }, int(m_screenViewport.Width), int(m_screenViewport.Height));
		m_postProcess = &postProcess;
		m_postProcessPSO = pso;
	}

	void RenderBase::CopyIndexForPicking(int mouseX, int mouseY, uint8_t* dest)
	{
		// Mouse Picking Text (TODO)
		{
			m_context->CopyResource(m_indexTempTexture.Get(), m_indexTexture.Get());

			// 콺 Ŀ ȼ Ѱ  Staging Texture 
			D3D11_BOX box;
			box.left = mouseX;
			box.right = mouseX + 1;
			box.top = mouseY;
			box.bottom = mouseY + 1;
			box.front = 0;
			box.back = 1;
			m_context->CopySubresourceRegion(m_indexStagingTexture.Get(), 0, 0, 0, 0,
				m_indexTempTexture.Get(), 0, &box);


			// GPU CPU  
			D3D11_MAPPED_SUBRESOURCE ms;
			m_context->Map(m_indexStagingTexture.Get(), 0, D3D11_MAP_READ, 0,
				&ms);
			// ȼ ϳ  
			memcpy(dest, ms.pData, sizeof(uint8_t) * 4);
			m_context->Unmap(m_indexStagingTexture.Get(), 0);

			//D3D11Utils::CopyFromStagingTexture(m_context, m_indexStagingTexture, sizeof(uint8_t) * 4, dest);
		}
	}

	void RenderBase::ClearDepthBuffer()
	{
		m_context->ClearDepthStencilView(m_defaultDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	}

	void RenderBase::SetShadowViewport(float width, float height)
	{
		// Shadow Mapping ´ Viewport 
		D3D11_VIEWPORT shadowViewport;
		ZeroMemory(&shadowViewport, sizeof(shadowViewport));
		shadowViewport.TopLeftX = 0;
		shadowViewport.TopLeftY = 0;
		// Shadow Map  Depth Buffer ػ󵵸  
		shadowViewport.Width = width;
		shadowViewport.Height = height;
		shadowViewport.MinDepth = 0.f;
		shadowViewport.MaxDepth = 1.f;

		m_context->RSSetViewports(1, &shadowViewport);
	}

	void RenderBase::SetShadowSRVs()
	{
		//std::vector<ID3D11ShaderResourceView*> shadowSRVs;
		//for (int i = 0; i < MAX_LIGHTS; ++i)
		//	shadowSRVs.emplace_back(m_shadowBuffers[i].GetSRV());

		//m_context->PSSetShaderResources(15, UINT(shadowSRVs.size()), shadowSRVs.data());
		ID3D11ShaderResourceView* shadowSRV = m_shadowArrayBuffer.GetSRV();
		m_context->PSSetShaderResources(15, 1, &shadowSRV);
	}

	void RenderBase::SetShadowMap(int idx)
	{
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		// RTS  
		context->OMSetRenderTargets(0, nullptr, m_shadowDSVs[idx].Get());
		context->ClearDepthStencilView(m_shadowDSVs[idx].Get(),
			D3D11_CLEAR_DEPTH, 1.0f, 0);
	}

	void RenderBase::DownsampleDepthToLowRes()
	{
		// Set compute shader pipeline
		SetPipelineState(computeCommon.depth.depthDownsampleCS);

		// Bind constant buffer
		m_context->CSSetConstantBuffers(0, 1, m_depthDownsampleCB.GetAddressOf());

		// Bind full-res depth as SRV (t0)
		ID3D11ShaderResourceView* srvs[] = { m_depthOnlyBuffer.GetSRV() };
		m_context->CSSetShaderResources(0, 1, srvs);

		// Bind UAV temp texture (u0) - NOT the actual depth buffer
		ID3D11UnorderedAccessView* uavs[] = { m_lowResDepthUAV.GetUAV() };
		m_context->CSSetUnorderedAccessViews(0, 1, uavs, nullptr);

		// Dispatch compute shader
		// Thread groups: divide by 8 (thread group size), round up
		UINT groupsX = (m_lowResWidth + 7) / 8;
		UINT groupsY = (m_lowResHeight + 7) / 8;
		m_context->Dispatch(groupsX, groupsY, 1);

		// Unbind resources to prevent hazards
		ID3D11ShaderResourceView* nullSRVs[] = { nullptr };
		m_context->CSSetShaderResources(0, 1, nullSRVs);

		ID3D11UnorderedAccessView* nullUAVs[] = { nullptr };
		m_context->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);

	}

	void RenderBase::SetLowResRender()
	{
		ZeroMemory(&m_lowResViewport, sizeof(D3D11_VIEWPORT));
		m_lowResViewport.TopLeftX = 0;
		m_lowResViewport.TopLeftY = 0;
		m_lowResViewport.Width = float(m_lowResWidth);
		m_lowResViewport.Height = float(m_lowResHeight);
		m_lowResViewport.MinDepth = 0.f;
		m_lowResViewport.MaxDepth = 1.f;

		m_context->RSSetViewports(1, &m_lowResViewport);

		float clearColor[4] = { 0.f, 0.f, 0.f, 0.f };
		m_context->ClearRenderTargetView(m_lowResParticleTexture.GetRTV(), clearColor);

		// Downsample scene depth into UAV for bilateral composite
		DownsampleDepthToLowRes();

		// Copy downsampled scene depth into DSV backing texture for hardware depth occlusion
		m_context->CopyResource(m_lowResDepth.GetTexture(), m_lowResDepthUAV.GetTexture());

		// Multiple Render Targets
		m_context->OMSetRenderTargets(1, m_lowResParticleTexture.GetAddressOfRTV(), m_lowResDSV.Get());
	}

	void RenderBase::RenderCompositeLowResParticles()
	{
		SetPipelineState(graphicsCommon.postProcess.particleCompositePSO);

		m_context->RSSetViewports(1, &m_screenViewport);
		m_context->OMSetRenderTargets(1, m_floatBuffer.GetAddressOfRTV(), nullptr);

		// Bind textures for Off-Screen Particle rendering
		// t0: Low-resolution particle color
		// t1: Full-resolution scene depth
		// t2: Low-resolution particle depth
		ID3D11ShaderResourceView* srvs[3] = {
			m_lowResParticleTexture.GetSRV(),
			m_depthOnlyBuffer.GetSRV(),
			m_lowResDepthUAV.GetSRV()
		};
		m_context->PSSetShaderResources(0, 3, srvs);

		m_context->IASetVertexBuffers(0, 1, m_compositeQuad->vertexBuffer.GetAddressOf(),
			&m_compositeQuad->stride, &m_compositeQuad->offset);
		m_context->IASetIndexBuffer(m_compositeQuad->indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		m_context->DrawIndexed(m_compositeQuad->indexCount, 0, 0);

		// Unbind SRV to prevent resource hazard
		ID3D11ShaderResourceView* nullSRVs[3] = { nullptr, nullptr, nullptr };
		m_context->PSSetShaderResources(0, 3, nullSRVs);
	}

	void RenderBase::ClearStencilBuffer()
	{
		// Stencil 0 Clear(ʱȭ)
		m_context->ClearDepthStencilView(m_defaultDSV.Get(), D3D11_CLEAR_STENCIL, 1.0f, 0);
	}
}