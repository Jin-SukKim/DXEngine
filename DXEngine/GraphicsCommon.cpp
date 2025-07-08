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
		// 거울에 반사되면 삼각형의 Winding(Index 순서)가 바뀌기 때문에 CCW로 그려줘야 함
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
		rastDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_NONE; // 양면
		rastDesc.FrontCounterClockwise = false;
		rastDesc.DepthClipEnable = true;
		rastDesc.MultisampleEnable = true;
		ThrowIfFailed(device->CreateRasterizerState(&rastDesc, solidBothRS.GetAddressOf()));
	}
	
	void GraphicsCommon::initDepthStencilStates(ComPtr<ID3D11Device>& device)
	{
		// D3D11_DEPTH_STENCIL_DESC 옵션 정리
		// https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_depth_stencil_desc
		// StencilRead/WriteMask: 예) uint8 중 어떤 비트를 사용할지

		// D3D11_DEPTH_STENCILOP_DESC 옵션 정리
		// https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_depth_stencilop_desc
		// StencilPassOp : 둘 다 pass일 때 할 일
		// StencilDepthFailOp : Stencil pass, Depth fail 일 때 할 일
		// StencilFailOp : 둘 다 fail 일 때 할 일

		// 기본 DSS
		D3D11_DEPTH_STENCIL_DESC dsDesc;
		ZeroMemory(&dsDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
		dsDesc.DepthEnable = true;
		// 경우에 따라 Depth Buffer를 껏다 켰다할 때 사용할 수 있음
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL;
		// Depth값이 더 작은걸 렌더링
		dsDesc.DepthFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS;
		// 기본 DS에선 Stencil 불필요
		dsDesc.StencilEnable = false;
		// Stencil Buffer는 8bit인데 이 중 어떤 bit를 사용할지
		dsDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
		dsDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
		// 앞면에 대해서 어떻게 작동할지 설정
		// StencilFailOp는 Stencil/Depth Test를 둘 다 Fail한 경우
		dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		// StencilDepthFailOp는 Stencil은 Pass, Depth는 Fail한 경우
		dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		// PassOP는 Stencil/Depth Test를 둘 다 pass하면 어떤 Operation을 할지 정의
		dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		// 뒷면에 대해 어떻게 작동할지 설정 (뒷면도 그릴 경우)
		dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
		dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

		ThrowIfFailed(device->CreateDepthStencilState(&dsDesc, drawDSS.GetAddressOf()));
	
		// Stencil에 1로 표기해주는 DSS (1이 아닌 다른 숫자로도 표기할 수 있음)
		dsDesc.DepthEnable = true; // 이미 그려진 물체 유지
		// 이미 그려진 물체들의 Depth값을 유지하기 위해 Depth Buffer는 write하지 않고 유지
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
		// Stencil 필수
		dsDesc.StencilEnable = true;
		// D3D11_DEFAULT_STENCIL_READ_MASK나 D3D11_DEFAULT_STENCIL_WRITE_MASK는 0xFF와 같은 값
		dsDesc.StencilReadMask = 0xFF; // 모든 Bit 다 사용
		dsDesc.StencilWriteMask = 0xFF; // 모든 bit 다 사용
		// 앞면에 대해서 어떻게 작동할지 설정
		dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		// 물체에 가려지지 않는 거울 부분만 Stencil Buffer에 값을 변환
		dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE; 
		// 거울 전체에 대해서는 Stencil test를 통과
		dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

		ThrowIfFailed(device->CreateDepthStencilState(&dsDesc, maskDSS.GetAddressOf()));

		// Stencil에 1로 표기된 경우에"만" 그리는 DSS 
		// (만약 다른 숫자로 Masking했다면 원하는 Masking에 맞는 숫자를 사용해 렌더링하면 됨)
		dsDesc.DepthEnable = true; // 거울 속을 다시 그릴때 필요
		dsDesc.StencilEnable = true; // Stencil Buffer 사용
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; // Masking된 부분을 그려줘야 하니 Depth Buffer도 사용
		// 거울에 반사된 세상을 그릴 것 이므로 거울보다 가까운 부분만 렌더링
		dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL; // <- 주의
		dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL; // 원하는 Masking 숫자와 값이 같은 경우

		ThrowIfFailed(device->CreateDepthStencilState(&dsDesc, drawMaskedDSS.GetAddressOf()));
	}
	
	void GraphicsCommon::initShaders(ComPtr<ID3D11Device>& device)
	{
		// InputLayouts

		// Default 기본 
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
	}
	
	void GraphicsCommon::initSamplers(ComPtr<ID3D11Device>& device)
	{
		// Texture sampler 만들기
		// 기본 Default
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

		sampleStates.emplace_back(linearWrapSS.Get()); // register(s0)
		sampleStates.emplace_back(linearClampSS.Get()); // register(s1)
	}
	
	void GraphicsCommon::initBlendStates(ComPtr<ID3D11Device>& device)
	{
		// 이미 그려져있는 화면과 어떻게 섞을지를 결정
		// Dest: 이미 그려져 있는 값들을 의미
		// Src: 픽셀 쉐이더가 계산한 값들을 의미
		// mirror blend에서는 Alpha Blending을 사용 (두 색을 원하는 비율, Alpha,로 섞어주기)
		D3D11_BLEND_DESC mirrorBlendDesc;
		ZeroMemory(&mirrorBlendDesc, sizeof(mirrorBlendDesc));
		mirrorBlendDesc.AlphaToCoverageEnable = true; // MSAA
		mirrorBlendDesc.IndependentBlendEnable = false; // 각각 RenderTarget에 따로 설정하려면 True로 설정
		// 개별 RenderTarget에 대해서 설정 (최대 8개)
		// 0번 RenderTarget이 메인 윈도우 (지금 1은 picking을 위한 Index Render Target으로 사용중)
		mirrorBlendDesc.RenderTarget[0].BlendEnable = true; 
		// 각각 Src, Dest에 곱해지는 비율로 두 비율을 합치면 1이 되는 값을 사용중
		// INV는 1에서 빼기를 한 비율을 의미 (1 - blendFactor)
		mirrorBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_INV_BLEND_FACTOR; // 거울 자체의 색
		// 이건 blendFactor 비율
		mirrorBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_BLEND_FACTOR; // 거울에 반사된 세상
		// D3D11_BLEND_OP_ADD는 Linear Interpolation 방법을 생각하면 됨 
		mirrorBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD; 

		// Alpha값들을 계산할 때 사용 (투명한 물체들이 누적되어 있는 경우 중요하게 사용됨)
		mirrorBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		mirrorBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
		mirrorBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

		// 필요하다면 RGBA 각각에 대해서도 조절 가능
		// RGBA에 대해서도 각각 조절할 수 있고, 반대로 색을 렌더링하고 싶지 않을때도 사용 가능
		mirrorBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		ThrowIfFailed(device->CreateBlendState(&mirrorBlendDesc, mirrorBS.GetAddressOf()));
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
		basic.wirePSO.rasterizerState = wireRS; // Solid에서 RS만 바뀜

		// Bounding Volume
		basic.boundPSO = basic.wirePSO; // Wire PSO 사용
		basic.boundPSO.pixelShader = colorPS; // Pixel Shader을 단색을 렌더링하는 Shader로 변경
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
		// Stencil Buffer에 Masking
		mirror.stencilMaskPSO = basic.solidPSO;
		mirror.stencilMaskPSO.inputLayout = skyboxIL; // position만 필요
		mirror.stencilMaskPSO.depthStencilState = maskDSS; // stencil buffer에 masking
		mirror.stencilMaskPSO.stencilRef = 1; // Stencil Buffer에 Masking할 값 (1로 Masking)
		mirror.stencilMaskPSO.vertexShader = depthOnlyVS;
		mirror.stencilMaskPSO.pixelShader = depthOnlyPS;

		// 반사되면 Index의 Winding이 반대가 됨
		mirror.reflectSolidPSO = basic.solidPSO;
		mirror.reflectSolidPSO.depthStencilState = drawMaskedDSS; // Masking된 Stencil Buffer의 위치에만 렌더링
		mirror.reflectSolidPSO.rasterizerState = solidCcwRS; // 반사됬으므로 FrontFace는 반시계 방향
		mirror.reflectSolidPSO.stencilRef = 1; // Stencil Buffer에 1로 Masking된 부분만 렌더링

		mirror.reflectWirePSO = mirror.reflectSolidPSO;
		mirror.reflectWirePSO.rasterizerState = wireCcwRS;
		mirror.reflectWirePSO.stencilRef = 1; // Stencil Buffer에 1로 Masking된 부분만 렌더링

		mirror.reflectSkyboxSolidPSO = skybox.solidPSO;
		mirror.reflectSkyboxSolidPSO.depthStencilState = drawMaskedDSS;
		mirror.reflectSkyboxSolidPSO.rasterizerState = solidCcwRS;
		mirror.reflectSkyboxSolidPSO.stencilRef = 1;

		mirror.reflectSkyboxWirePSO = mirror.reflectSkyboxSolidPSO;
		mirror.reflectSkyboxWirePSO.rasterizerState = wireCcwRS;
		mirror.reflectSkyboxWirePSO.stencilRef = 1;

		// 1로 Masking된 Mirror 부분만 BlendState를 활용해서 그려주기
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
		depth.depthOnlyPSO.inputLayout = skyboxIL; // position만 필요
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
		// 약간 다른 Topology를 사용 (POINTLIST이므로 렌더링시 Draw()를 사용)
		basic.tessellationQuadPSO.primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST;;
	}
}