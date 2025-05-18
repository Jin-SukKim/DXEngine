#include "pch.h"
#include "Scene.h"
#include "CameraActor.h"
#include "TransformComponent.h"

#include "TriangleActor.h"

namespace DE {
	Scene::Scene(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context) : m_device(device), m_context(context)
	{
		m_mainCamera = std::make_shared<CameraActor>(L"MainCamera");
		TransformComponent* tr = m_mainCamera->GetComponent<TransformComponent>();
		if (tr) {
			tr->SetPos(Vector3(0.f, 0.f, -10.f));
		}

		// 공통으로 쓰이는 Constant buffer
		D3D11Utils::CreateConstantBuffer(m_device, m_globalConstsCPU, m_globalConstsGPU);

		triangle = std::make_shared<TriangleActor>(L"Temp");
	}

	void Scene::Initialize() {
		// DirectX는 Row-Major인데 HLSL는 Column-Major이므로 Transpose
		m_globalConstsCPU.view = m_mainCamera->GetViewMatrix().Transpose();
		m_globalConstsCPU.proj = m_mainCamera->GetProjMatrix().Transpose();

		triangle->Initialize(m_device);
	}

	void Scene::Update() {
		D3D11Utils::UpdateBuffer(m_context, m_globalConstsCPU, m_globalConstsGPU);

		triangle->Update(m_context, 0.01f);
	}

	void Scene::Render() {
		// Global Constants을 Shader에서 사용할 수 있도록 설정
		SetGlobalConsts();

		triangle->Render(m_context);
	}
	void Scene::SetGlobalConsts()
	{
		m_context->VSSetConstantBuffers(0, 1, m_globalConstsGPU.GetAddressOf());
		m_context->PSSetConstantBuffers(0, 1, m_globalConstsGPU.GetAddressOf());
	}
}