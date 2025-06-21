#include "pch.h"
#include "SkyboxActor.h"

#include "MeshData.h"
#include "GeometryGenerator.h"
#include "ModelComponent.h"
#include "RenderBase.h"

namespace DE {
	SkyboxActor::SkyboxActor(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const std::wstring& name) : Super(device, context, name)
	{
		MeshData cubeMap = GeometryGenerator::MakeBox(40.f);
		// IBL용 Cube는 박스 안에서 바라보기 때문에 Index 순서를 뒤집어주기
		std::reverse(cubeMap.indices.begin(), cubeMap.indices.end());
		m_sky = AddComponent<ModelComponent>(device, name);
		m_sky->SetModel(device, context, cubeMap);

		// 기본으로 적용할 Cubemap
		//SetCubeMaps(device, L"../Assets/Textures/Cubemaps/HDRI/", 
		//	L"SampleEnvHDR.dds",	L"SampleSpecularHDR.dds", 
		//	L"SampleDiffuseHDR.dds",	L"SampleBrdf.dds");
		SetCubeMaps(device, L"../Assets/Textures/Cubemaps/HDRI/", 
			L"MyCubesEnvHDR.dds",	L"MyCubesSpecularHDR.dds", 
			L"MyCubesDiffuseHDR.dds",	L"MyCubesBrdf.dds");
	}

	void SkyboxActor::Initialize()
	{
		Super::Initialize();
	}

	void SkyboxActor::Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime)
	{
		Super::Update(context, deltaTime);
	}

	void SkyboxActor::Render(RenderBase& renderer)
	{
		renderer.SetPipelineState(RenderBase::graphicsCommon.skybox.solidPSO);
		RenderComponent(renderer.GetContext(), ComponentType::Model);
	}

	void SkyboxActor::SetCubeMaps(ComPtr<ID3D11Device>& device, std::wstring basePath, std::wstring envFilename, std::wstring specularFilename, std::wstring irradianceFilename, std::wstring brdfFilename)
	{
		// BRDF LookUp Table은 CubeMap이 아니라 2D Texture
		D3D11Utils::CreateDDSTexture(device, basePath + envFilename, true, m_envSRV);
		D3D11Utils::CreateDDSTexture(device, basePath + specularFilename, true, m_specularSRV);
		D3D11Utils::CreateDDSTexture(device, basePath + irradianceFilename, true, m_irradianceSRV);
		D3D11Utils::CreateDDSTexture(device, basePath + brdfFilename, false, m_brdfSRV);
	}

	void SkyboxActor::SetCommonSRVs(ComPtr<ID3D11DeviceContext>& context)
	{	
		std::vector<ID3D11ShaderResourceView*> commonSRVs = {
			m_envSRV.Get(),	m_specularSRV.Get(), m_irradianceSRV.Get(),	m_brdfSRV.Get()
		};
		// 공통으로 사용할 Texture들을 "Common.hlsli"에서 register(t10)부터 시작
		context->PSSetShaderResources(10, UINT(commonSRVs.size()), commonSRVs.data());
	}
}