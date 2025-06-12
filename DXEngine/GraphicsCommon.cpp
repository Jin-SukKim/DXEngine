#include "pch.h"
#include "GraphicsCommon.h"

namespace DE {
	ComPtr<ID3D11PixelShader>  GraphicsCommon::TreeBillboardPS = nullptr;

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

		// WireRS
		rastDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_WIREFRAME;
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
		dsDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
		dsDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
		// 앞면에 대해서 어떻게 작동할지 설정
		dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		// 뒷면에 대해 어떻게 작동할지 설정 (뒷면도 그릴 경우)
		dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
		dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

		ThrowIfFailed(device->CreateDepthStencilState(&dsDesc, drawDDS.GetAddressOf()));
	}
	
	void GraphicsCommon::initShaders(ComPtr<ID3D11Device>& device)
	{
		// InputLayouts

		// Default 기본 
		std::vector<D3D11_INPUT_ELEMENT_DESC> basicIEs = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 6, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};
		D3D11Utils::CreateVSAndIL(device, L"BasicVS.hlsl", basicIEs, basicVS, basicIL);
		D3D11Utils::CreatePS(device, L"BasicPS.hlsl", basicPS);
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

		// Billboard
		std::vector<D3D11_INPUT_ELEMENT_DESC> billboardIEs = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
		};
		D3D11Utils::CreateVSAndIL(device, L"BillboardVS.hlsl", billboardIEs, billboardVS, billboardIL);
		D3D11Utils::CreateGS(device, L"BillboardGS.hlsl", billboardGS);
		D3D11Utils::CreatePS(device, L"BillboardPS.hlsl", billboardPS);
		D3D11Utils::CreatePS(device, L"TreeBillboardPS.hlsl", TreeBillboardPS);
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
	}
	
	void GraphicsCommon::initPipelineStates(ComPtr<ID3D11Device>& device)
	{
		// Basic (Default Solid)
		basic.solidPSO.inputLayout = basicIL;
		basic.solidPSO.vertexShader = basicVS;
		basic.solidPSO.pixelShader = basicPS;
		basic.solidPSO.rasterizerState = solidRS;
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

		// PostProcessing
		postProcess.bloomPSO = basic.solidPSO;
		postProcess.bloomPSO.inputLayout = samplingIL;
		postProcess.bloomPSO.vertexShader = samplingVS;
		postProcess.bloomPSO.rasterizerState = postProcessRS;

		// Billboard
		billboard.solidPSO = basic.solidPSO;
		billboard.solidPSO.vertexShader = billboardVS;
		billboard.solidPSO.geometryShader = billboardGS;
		billboard.solidPSO.pixelShader = billboardPS;
		billboard.solidPSO.primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
		billboard.solidPSO.rasterizerState = solidBothRS;

	}
}