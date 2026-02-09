#include "pch.h"
#include "GraphicsCommon.h"

namespace DE {
	void GraphicsCommon::InitCommonStates(ComPtr<ID3D11Device>& device)
	{
		initRasterizerStates(device);
		initDepthStencilStates(device);
		initShaders(device);
		initSamplers(device);
		initBlendStates(device);

		initPipelineStates(device);
	}

	void GraphicsCommon::initRasterizerStates(ComPtr<ID3D11Device>& device)
	{
		// SolidRS
		D3D11_RASTERIZER_DESC rastDesc;
		ZeroMemory(&rastDesc, sizeof(D3D11_RASTERIZER_DESC));
		rastDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
		rastDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_BACK;
		rastDesc.FrontCounterClockwise = false;
		rastDesc.DepthClipEnable = true; // zNear, zFar
		rastDesc.MultisampleEnable = true;

		ThrowIfFailed(device->CreateRasterizerState(&rastDesc, solidRS.GetAddressOf()));

		// SolidCcwRS
		// ſ￡ ݻǸ ﰢ Winding(Index ) ٲ  CCW ׷ 
		rastDesc.FrontCounterClockwise = true; 
		ThrowIfFailed(device->CreateRasterizerState(&rastDesc, solidCcwRS.GetAddressOf()));

		// WireCcwRS
		rastDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_WIREFRAME;
		ThrowIfFailed(device->CreateRasterizerState(&rastDesc, wireCcwRS.GetAddressOf()));

		// WireRS
		rastDesc.FrontCounterClockwise = false; 
		ThrowIfFailed(device->CreateRasterizerState(&rastDesc, wireRS.GetAddressOf()));

		// post-process
		ZeroMemory(&rastDesc, sizeof(D3D11_RASTERIZER_DESC));
		rastDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
		rastDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_NONE;
		rastDesc.FrontCounterClockwise = false;
		rastDesc.DepthClipEnable = false;
		ThrowIfFailed(device->CreateRasterizerState(&rastDesc, postProcessRS.GetAddressOf()));

		// Both RS
		ZeroMemory(&rastDesc, sizeof(D3D11_RASTERIZER_DESC));
		rastDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
		rastDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_NONE; // 
		rastDesc.FrontCounterClockwise = false;
		rastDesc.DepthClipEnable = true;
		rastDesc.MultisampleEnable = true;
		ThrowIfFailed(device->CreateRasterizerState(&rastDesc, solidBothRS.GetAddressOf()));
	}
	
	void GraphicsCommon::initDepthStencilStates(ComPtr<ID3D11Device>& device)
	{
		// D3D11_DEPTH_STENCIL_DESC ɼ 
		// https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_depth_stencil_desc
		// StencilRead/WriteMask: ) uint8   Ʈ 

		// D3D11_DEPTH_STENCILOP_DESC ɼ 
		// https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_depth_stencilop_desc
		// StencilPassOp :   pass   
		// StencilDepthFailOp : Stencil pass, Depth fail    
		// StencilFailOp :   fail    

		// ⺻ DSS
		D3D11_DEPTH_STENCIL_DESC dsDesc;
		ZeroMemory(&dsDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
		dsDesc.DepthEnable = true;
		// 쿡  Depth Buffer  ״    
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL;
		// Depth   
		dsDesc.DepthFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS;
		// ⺻ DS Stencil ʿ
		dsDesc.StencilEnable = false;
		// Stencil Buffer 8bitε    bit 
		dsDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
		dsDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
		// ո鿡 ؼ  ۵ 
		// StencilFailOp Stencil/Depth Test   Fail 
		dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		// StencilDepthFailOp Stencil Pass, Depth Fail 
		dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		// PassOP Stencil/Depth Test   passϸ  Operation  
		dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		// ޸鿡   ۵  (޸鵵 ׸ )
		dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
		dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

		ThrowIfFailed(device->CreateDepthStencilState(&dsDesc, drawDSS.GetAddressOf()));
	
		// Stencil 1 ǥִ DSS (1 ƴ ٸ ڷε ǥ  )
		dsDesc.DepthEnable = true; // ̹ ׷ ü 
		// ̹ ׷ ü Depth ϱ  Depth Buffer write ʰ 
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
		// Stencil ʼ
		dsDesc.StencilEnable = true;
		// D3D11_DEFAULT_STENCIL_READ_MASK D3D11_DEFAULT_STENCIL_WRITE_MASK 0xFF  
		dsDesc.StencilReadMask = 0xFF; //  Bit  
		dsDesc.StencilWriteMask = 0xFF; //  bit  
		// ո鿡 ؼ  ۵ 
		dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		// ü  ʴ ſ κи Stencil Buffer  ȯ
		dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE; 
		// ſ ü ؼ Stencil test 
		dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

		ThrowIfFailed(device->CreateDepthStencilState(&dsDesc, maskDSS.GetAddressOf()));

		// Stencil 1 ǥ 쿡"" ׸ DSS 
		// ( ٸ ڷ Maskingߴٸ ϴ Masking ´ ڸ  ϸ )
		dsDesc.DepthEnable = true; // ſ  ٽ ׸ ʿ
		dsDesc.StencilEnable = true; // Stencil Buffer 
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; // Masking κ ׷ ϴ Depth Buffer 
		// ſ￡ ݻ  ׸  ̹Ƿ ſﺸ  κи 
		dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL; // <- 
		dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL; // ϴ Masking ڿ   

		ThrowIfFailed(device->CreateDepthStencilState(&dsDesc, drawMaskedDSS.GetAddressOf()));

		dsDesc.DepthEnable = true;
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // ߿: 0 Ͽ  
		dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
		dsDesc.StencilEnable = false;

		ThrowIfFailed(device->CreateDepthStencilState(&dsDesc, particleDDS.GetAddressOf()));
	}
	
	void GraphicsCommon::initShaders(ComPtr<ID3D11Device>& device)
	{
		// InputLayouts

		// Default ⺻ 
		std::vector<D3D11_INPUT_ELEMENT_DESC> basicIEs = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 6, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};
		D3D11Utils::CreateVSAndIL(device, L"BasicVS.hlsl", basicIEs, basicVS, basicIL);
		//D3D11Utils::CreatePS(device, L"BasicPS.hlsl", basicPS);
		D3D11Utils::CreatePS(device, L"UnrealPBR.hlsl", basicPS);

		// Bounding Volume
		D3D11Utils::CreatePS(device, L"ColorPS.hlsl", colorPS);

		// Normal Vector
		D3D11Utils::CreateVSAndIL(device, L"NormalVS.hlsl", basicIEs, normalVS, basicIL);
		D3D11Utils::CreateGS(device, L"NormalGS.hlsl", normalGS);
		D3D11Utils::CreatePS(device, L"NormalPS.hlsl", normalPS);

		// Particle
		D3D11Utils::CreateVSAndIL(device, L"ParticleMeshVS.hlsl", basicIEs, particleMeshVS, basicIL);

		// Skybox
		std::vector<D3D11_INPUT_ELEMENT_DESC> skyboxIEs = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
		};
		D3D11Utils::CreateVSAndIL(device, L"SkyboxVS.hlsl", skyboxIEs, skyboxVS, skyboxIL);
		D3D11Utils::CreatePS(device, L"SkyboxPS.hlsl", skyboxPS);

		// Mirror - DepthOnly
		D3D11Utils::CreateVSAndIL(device, L"DepthOnlyVS.hlsl", skyboxIEs, depthOnlyVS, skyboxIL);
		D3D11Utils::CreatePS(device, L"DepthOnlyPS.hlsl", depthOnlyPS);

		// PostProcessing
		std::vector<D3D11_INPUT_ELEMENT_DESC> postProcessIEs = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 6, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		D3D11Utils::CreateVSAndIL(device, L"SamplingVS.hlsl", postProcessIEs, samplingVS, samplingIL);
		// Bloom Filter
		D3D11Utils::CreatePS(device, L"BloomDownPS.hlsl", bloomDownPS);
		D3D11Utils::CreatePS(device, L"BloomUpPS.hlsl", bloomUpPS);
		D3D11Utils::CreatePS(device, L"CombinePS.hlsl", combinePS);
		// ToneMapping
		D3D11Utils::CreatePS(device, L"ToneMappingPS.hlsl", toneMappingPS);
		// Copy
		D3D11Utils::CreatePS(device, L"CopyFilterPS.hlsl", copyPS);
		// Depth
		D3D11Utils::CreatePS(device, L"DepthPS.hlsl", depthPS);
		// Fog
		D3D11Utils::CreatePS(device, L"FogEffectPS.hlsl", fogPS);
		// Halo
		D3D11Utils::CreatePS(device, L"HaloPS.hlsl", haloPS);

		// Billboard
		std::vector<D3D11_INPUT_ELEMENT_DESC> billboardIEs = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
		};
		D3D11Utils::CreateVSAndIL(device, L"BillboardVS.hlsl", billboardIEs, billboardVS, billboardIL);
		D3D11Utils::CreateGS(device, L"BillboardGS.hlsl", billboardGS);
		D3D11Utils::CreatePS(device, L"BillboardPS.hlsl", billboardPS);
		D3D11Utils::CreatePS(device, L"TreeBillboardPS.hlsl", TreeBillboardPS);

		// Tessellation Quad
		D3D11Utils::CreateVSAndIL(device, L"tessellationQuadVS.hlsl", billboardIEs, tessellationQuadVS, billboardIL);
		D3D11Utils::CreateHS(device, L"tessellationQuadHS.hlsl", tessellationQuadHS);
		D3D11Utils::CreateDS(device, L"tessellationQuadDS.hlsl", tessellationQuadDS);
		D3D11Utils::CreatePS(device, L"tessellationQuadPS.hlsl", tessellationQuadPS);


		// Particle
		std::vector<D3D11_INPUT_ELEMENT_DESC> particleIEs = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
		}; // Dummy (δ Structured Buffer )
		D3D11Utils::CreateVSAndIL(device, L"ParticleVS.hlsl", particleIEs, particleVS, particleIL);
		D3D11Utils::CreateGS(device, L"ParticleGS.hlsl", particleGS);
		D3D11Utils::CreatePS(device, L"ParticlePS.hlsl", particlePS);
		D3D11Utils::CreatePS(device, L"ParticlePBR.hlsl", particlePbrPS);

		// Billboard Instancing (GS 없음) - Vertex 구조체 사용
		std::vector<D3D11_INPUT_ELEMENT_DESC> particleBillboardIEs = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0}
		};
		D3D11Utils::CreateVSAndIL(device, L"ParticleBillboardVS.hlsl", particleBillboardIEs, particleBillboardVS, particleBillboardIL);
	}
	
	void GraphicsCommon::initSamplers(ComPtr<ID3D11Device>& device)
	{
		// Texture sampler 
		// ⺻ Default
		D3D11_SAMPLER_DESC sampDesc;
		ZeroMemory(&sampDesc, sizeof(sampDesc));
		sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; // Linear Interpolation
		// Wrap
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampDesc.MinLOD = 0;
		sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

		// Create the Sample State
		device->CreateSamplerState(&sampDesc, linearWrapSS.GetAddressOf());

		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		device->CreateSamplerState(&sampDesc, linearClampSS.GetAddressOf());
		
		// shadowPointSS
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER; // ڸ Ѿ ׳ ū z ȯ
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
		// ڸ    ̸ 
		// (Shaow Map ī޶    ֱ )
		sampDesc.BorderColor[0] = 1.0f; // ū Z
		sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		device->CreateSamplerState(&sampDesc, shadowPointSS.GetAddressOf());

		// shadowCompareSS, ̴ ȿ SamplerComparisonState
		// Filter = "_COMPARISON_" 
		// https://www.gamedev.net/forums/topic/670575-uploading-samplercomparisonstate-in-hlsl/
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
		sampDesc.BorderColor[0] = 100.0f; // ū Z
		sampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
		sampDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
		device->CreateSamplerState(&sampDesc, shadowCompareSS.GetAddressOf());

		sampleStates.emplace_back(linearWrapSS.Get()); // register(s0)
		sampleStates.emplace_back(linearClampSS.Get()); // register(s1)
		sampleStates.emplace_back(shadowPointSS.Get()); // register(s2)
		sampleStates.emplace_back(shadowCompareSS.Get()); // register(s3)
	}
	
	void GraphicsCommon::initBlendStates(ComPtr<ID3D11Device>& device)
	{
		// ̹ ׷ִ ȭ   
		// Dest: ̹ ׷ ִ  ǹ
		// Src: ȼ ̴   ǹ
		// mirror blend Alpha Blending  (  ϴ , Alpha, ֱ)
		D3D11_BLEND_DESC mirrorBlendDesc;
		ZeroMemory(&mirrorBlendDesc, sizeof(mirrorBlendDesc));
		mirrorBlendDesc.AlphaToCoverageEnable = true; // MSAA
		mirrorBlendDesc.IndependentBlendEnable = false; //  RenderTarget  Ϸ True 
		//  RenderTarget ؼ  (ִ 8)
		// 0 RenderTarget   ( 1 picking  Index Render Target )
		mirrorBlendDesc.RenderTarget[0].BlendEnable = true; 
		//  Src, Dest     ġ 1 Ǵ  
		// INV 1 ⸦   ǹ (1 - blendFactor)
		mirrorBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_INV_BLEND_FACTOR; // ſ ü 
		// ̰ blendFactor 
		mirrorBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_BLEND_FACTOR; // ſ￡ ݻ 
		// D3D11_BLEND_OP_ADD Linear Interpolation  ϸ  
		mirrorBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD; 

		// Alpha    ( ü Ǿ ִ  ߿ϰ )
		mirrorBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		mirrorBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
		mirrorBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

		// ʿϴٸ RGBA  ؼ  
		// RGBA ؼ    ְ, ݴ  ϰ    
		mirrorBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		ThrowIfFailed(device->CreateBlendState(&mirrorBlendDesc, mirrorBS.GetAddressOf()));

		D3D11_BLEND_DESC blendDesc;
		ZeroMemory(&blendDesc, sizeof(blendDesc));
		blendDesc.AlphaToCoverageEnable = false; // MSAA
		blendDesc.IndependentBlendEnable = false;
		blendDesc.RenderTarget[0].BlendEnable = true;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE; // INV ƴ
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask =
			D3D11_COLOR_WRITE_ENABLE_ALL;
		ThrowIfFailed(
			device->CreateBlendState(&blendDesc, accumulateBS.GetAddressOf()));

		
		// SrcBlend ONE  (̹ ̴ ĸ ؿ ̹Ƿ)
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		ThrowIfFailed(
			device->CreateBlendState(&blendDesc, alphaBS.GetAddressOf()));
	}
	
	void GraphicsCommon::initPipelineStates(ComPtr<ID3D11Device>& device)
	{
		// Basic (Default Solid)
		basic.solidPSO.inputLayout = basicIL;
		basic.solidPSO.vertexShader = basicVS;
		basic.solidPSO.pixelShader = basicPS;
		basic.solidPSO.rasterizerState = solidRS;
		basic.solidPSO.depthStencilState = drawDSS;
		basic.solidPSO.primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		// Basic Wire
		basic.wirePSO = basic.solidPSO;
		basic.wirePSO.rasterizerState = wireRS; // Solid RS ٲ

		// Bounding Volume
		basic.boundPSO = basic.wirePSO; // Wire PSO 
		basic.boundPSO.pixelShader = colorPS; // Pixel Shader ܻ ϴ Shader 
		basic.boundPSO.primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;

		// Normal
		normal.solidPSO = basic.solidPSO;
		normal.solidPSO.vertexShader = normalVS;
		normal.solidPSO.geometryShader = normalGS;
		normal.solidPSO.pixelShader = normalPS;
		normal.solidPSO.primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;

		// Skybox
		skybox.solidPSO = basic.solidPSO;
		skybox.solidPSO.inputLayout = skyboxIL;
		skybox.solidPSO.vertexShader = skyboxVS;
		skybox.solidPSO.pixelShader = skyboxPS;

		skybox.wirePSO = skybox.solidPSO;
		skybox.wirePSO.rasterizerState = wireRS;

		// Mirror
		// Stencil Buffer Masking
		mirror.stencilMaskPSO = basic.solidPSO;
		mirror.stencilMaskPSO.inputLayout = skyboxIL; // position ʿ
		mirror.stencilMaskPSO.depthStencilState = maskDSS; // stencil buffer masking
		mirror.stencilMaskPSO.stencilRef = 1; // Stencil Buffer Masking  (1 Masking)
		mirror.stencilMaskPSO.vertexShader = depthOnlyVS;
		mirror.stencilMaskPSO.pixelShader = depthOnlyPS;

		// ݻǸ Index Winding ݴ밡 
		mirror.reflectSolidPSO = basic.solidPSO;
		mirror.reflectSolidPSO.depthStencilState = drawMaskedDSS; // Masking Stencil Buffer ġ 
		mirror.reflectSolidPSO.rasterizerState = solidCcwRS; // ݻǷ FrontFace ݽð 
		mirror.reflectSolidPSO.stencilRef = 1; // Stencil Buffer 1 Masking κи 

		mirror.reflectWirePSO = mirror.reflectSolidPSO;
		mirror.reflectWirePSO.rasterizerState = wireCcwRS;
		mirror.reflectWirePSO.stencilRef = 1; // Stencil Buffer 1 Masking κи 

		mirror.reflectSkyboxSolidPSO = skybox.solidPSO;
		mirror.reflectSkyboxSolidPSO.depthStencilState = drawMaskedDSS;
		mirror.reflectSkyboxSolidPSO.rasterizerState = solidCcwRS;
		mirror.reflectSkyboxSolidPSO.stencilRef = 1;

		mirror.reflectSkyboxWirePSO = mirror.reflectSkyboxSolidPSO;
		mirror.reflectSkyboxWirePSO.rasterizerState = wireCcwRS;
		mirror.reflectSkyboxWirePSO.stencilRef = 1;

		// 1 Masking Mirror κи BlendState Ȱؼ ׷ֱ
		mirror.mirrorBlendSolidPSO = basic.solidPSO;
		mirror.mirrorBlendSolidPSO.blendState = mirrorBS;
		mirror.mirrorBlendSolidPSO.depthStencilState = drawMaskedDSS;
		mirror.mirrorBlendSolidPSO.stencilRef = 1;

		mirror.mirrorBlendWirePSO = basic.wirePSO;
		mirror.mirrorBlendWirePSO.blendState = mirrorBS;
		mirror.mirrorBlendWirePSO.depthStencilState = drawMaskedDSS;
		mirror.mirrorBlendWirePSO.stencilRef = 1;

		// Depth
		depth.depthOnlyPSO = basic.solidPSO;
		depth.depthOnlyPSO.inputLayout = skyboxIL; // position ʿ
		depth.depthOnlyPSO.vertexShader = depthOnlyVS;
		depth.depthOnlyPSO.pixelShader = depthOnlyPS;

		// PostProcessing
		postProcess.basicPSO = basic.solidPSO;
		postProcess.basicPSO.inputLayout = samplingIL;
		postProcess.basicPSO.vertexShader = samplingVS;
		postProcess.basicPSO.rasterizerState = postProcessRS;

		// Billboard
		billboard.solidPSO = basic.solidPSO;
		billboard.solidPSO.vertexShader = billboardVS;
		billboard.solidPSO.geometryShader = billboardGS;
		billboard.solidPSO.pixelShader = billboardPS;
		billboard.solidPSO.primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
		billboard.solidPSO.rasterizerState = solidBothRS;

		mirror.reflectBillboardSolidPSO = billboard.solidPSO;
		mirror.reflectBillboardSolidPSO.depthStencilState = drawMaskedDSS;
		mirror.reflectBillboardSolidPSO.rasterizerState = solidCcwRS;
		mirror.reflectBillboardSolidPSO.stencilRef = 1;

		// Tessellation Quad
		basic.tessellationQuadPSO = basic.solidPSO;
		basic.tessellationQuadPSO.vertexShader = tessellationQuadVS;
		basic.tessellationQuadPSO.hullShader = tessellationQuadHS;
		basic.tessellationQuadPSO.domainShader = tessellationQuadDS;
		basic.tessellationQuadPSO.pixelShader = tessellationQuadPS;
		// ణ ٸ Topology  (POINTLIST̹Ƿ  Draw() )
		basic.tessellationQuadPSO.primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST;;

		// Particle System
		particle.animPSO = basic.solidPSO;
		particle.animPSO.vertexShader = particleVS;
		particle.animPSO.geometryShader = particleGS;
		particle.animPSO.pixelShader = particlePS;
		particle.animPSO.primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
		particle.animPSO.rasterizerState = solidBothRS;
		particle.animPSO.blendState = accumulateBS;
		particle.animPSO.depthStencilState = particleDDS;

		mirror.reflectEffectSolidPSO = particle.animPSO;
		mirror.reflectEffectSolidPSO.depthStencilState = drawMaskedDSS;
		mirror.reflectEffectSolidPSO.rasterizerState = solidCcwRS;
		mirror.reflectEffectSolidPSO.stencilRef = 1;

		particle.meshPSO = basic.solidPSO;
		particle.meshPSO.vertexShader = particleMeshVS;
		//particle.meshPSO.pixelShader = particlePbrPS;

		// Billboard Instancing (GS 없음)
		particle.billboardInstancedPSO = particle.animPSO;
		particle.billboardInstancedPSO.inputLayout = particleBillboardIL;
		particle.billboardInstancedPSO.vertexShader = particleBillboardVS;
		particle.billboardInstancedPSO.geometryShader = nullptr; // GS 제거
		particle.billboardInstancedPSO.primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}
}