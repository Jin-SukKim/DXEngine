#include "pch.h"
#include "SkyboxActor.h"

#include "MeshData.h"
#include "GeometryGenerator.h"
#include "ModelComponent.h"
#include "RenderBase.h"

namespace DE {
	SkyboxActor::SkyboxActor(const std::wstring& name) : Super(name)
	{
		MeshData cubeMap = GeometryGenerator::MakeBox(50.f);
		// IBL용 Cube는 박스 안에서 바라보기 때문에 Index 순서를 뒤집어주기
		std::reverse(cubeMap.indices.begin(), cubeMap.indices.end());
		m_sky = AddComponent<ModelComponent>(name);
		m_sky->SetModel(cubeMap);

		// 기본으로 적용할 Cubemap
		SetCubeMaps(L"../Assets/Textures/Cubemaps/HDRI/", 
			L"SampleEnvHDR.dds",	L"SampleSpecularHDR.dds", 
			L"SampleDiffuseHDR.dds",	L"SampleBrdf.dds");
		//SetCubeMaps(L"../Assets/Textures/Cubemaps/HDRI/", 
		//	L"clear_pureskyEnvHDR.dds",	L"clear_pureskySpecularHDR.dds", 
		//	L"clear_pureskyDiffuseHDR.dds",	L"clear_pureskyBrdf.dds");
	}

	void SkyboxActor::Initialize()
	{
		Super::Initialize();
	}

	void SkyboxActor::Update(const float& deltaTime)
	{
		Super::Update(deltaTime);
	}

	void SkyboxActor::Render()
	{
		Super::Render();
	}

	void SkyboxActor::SetCubeMaps(std::wstring basePath, std::wstring envFilename, std::wstring specularFilename, std::wstring irradianceFilename, std::wstring brdfFilename)
	{
		ComPtr<ID3D11Device>& device = GET_SINGLE(RenderBase)->GetDevice();
		// BRDF LookUp Table은 CubeMap이 아니라 2D Texture
		D3D11Utils::CreateDDSTexture(device, basePath + envFilename, true, m_envSRV);
		D3D11Utils::CreateDDSTexture(device, basePath + specularFilename, true, m_specularSRV);
		D3D11Utils::CreateDDSTexture(device, basePath + irradianceFilename, true, m_irradianceSRV);
		D3D11Utils::CreateDDSTexture(device, basePath + brdfFilename, false, m_brdfSRV);
	}

	void SkyboxActor::SetCommonSRVs()
	{
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		std::vector<ID3D11ShaderResourceView*> commonSRVs = {
			m_envSRV.Get(),	m_specularSRV.Get(), m_irradianceSRV.Get(),	m_brdfSRV.Get()
		};
		// 공통으로 사용할 Texture들을 "Common.hlsli"에서 register(t10)부터 시작
		context->PSSetShaderResources(10, UINT(commonSRVs.size()), commonSRVs.data());
	}
	void SkyboxActor::SetCommonSRVToNull()
	{
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		std::vector<ID3D11ShaderResourceView*> commonSRVs = {
			NULL, NULL, NULL, NULL
		};
		// 공통으로 사용할 Texture들을 "Common.hlsli"에서 register(t10)부터 시작
		context->PSSetShaderResources(10, UINT(commonSRVs.size()), commonSRVs.data());
	}
}