#include "pch.h"
#include "Scene.h"
#include "CameraActor.h"
#include "SkyboxActor.h"
#include "TransformComponent.h"

#include "AppBase.h"
#include "InputManager.h"

#include "SampleActor.h"
#include "RenderBase.h"

#include "BloomEffect.h"

namespace DE {
	Scene::Scene(RenderBase& renderer)
	{
		ComPtr<ID3D11Device>& device = renderer.GetDevice();
		ComPtr<ID3D11DeviceContext>& context = renderer.GetContext();

		// 공통으로 쓰이는 Constant buffer
		D3D11Utils::CreateConstantBuffer(device, m_globalConstsCPU, m_globalConstsGPU);

		// Scene 공통 Actor
		{
			m_mainCamera = std::make_shared<CameraActor>(device, L"MainCamera");
			m_skybox = std::make_shared<SkyboxActor>(device, L"Skybox");
		}
		action = InputAxisAction(w, s);

		triangle = std::make_shared<SampleActor>(device, L"Temp");

		m_blommPostProcess = std::make_shared<BloomEffect>();
		m_blommPostProcess->SetFilterLevel(4);
		renderer.SetPostProcess(*m_blommPostProcess.get(), RenderBase::graphicsCommon.postProcess.bloomPSO);
	}

	void Scene::Initialize() {
		// 조명 설정
		{
			// 현재 조명은 최대 개수 3개
			// Spot Light
			m_globalConstsCPU.lights[0].radiance = Vector3(1.0f);
			m_globalConstsCPU.lights[0].position = Vector3(0.0f, 0.0f, -2.0f);  // 위에서 비스듬히
			m_globalConstsCPU.lights[0].direction = Vector3(0.0f, 0.0f, 1.0f);  // 아래 방향으로
			m_globalConstsCPU.lights[0].spotPower = 10.0f;                      // 좀 더 집중된 빛
			m_globalConstsCPU.lights[0].fallOffStart = 0.0f;
			m_globalConstsCPU.lights[0].fallOffEnd = 10.0f;
			m_globalConstsCPU.lights[0].radius = 0.f;
			m_globalConstsCPU.lights[0].type = LIGHT_SPOT;

			// 조명 1개만 사용하고 나머지는 사용하지 않음
			m_globalConstsCPU.lights[1].type = LIGHT_OFF;
			m_globalConstsCPU.lights[2].type = LIGHT_OFF;
		}

		// 조명 위치 표시
		{
			// 조명을 원 같은 형태가 있는 것으로 화면에 렌더링할때 사용할 Actor
			for (int i = 0; i < MAX_LIGHTS; ++i) {
				
			}
		}

		// 카메라 위치 표시
		{
			m_mainCamera->Initialize();
			TransformComponent* tr = m_mainCamera->GetComponent<TransformComponent>();
			if (tr) {
				tr->SetPos(Vector3(0.f, 0.f, -2.f));
			}
		}

		// Skybox
		{
			m_skybox->Initialize();
		}

		// 입력 Bind
		{
			AppBase::GetInputManager().BindInputAxis(axis, action, this, &Scene::MoveForward);
		}

		triangle->Initialize();
		TransformComponent* tr = triangle->GetComponent<TransformComponent>();
		if (tr) {
			tr->SetScale(Vector3(0.5f));
		}
	}

	void Scene::Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime) {
		// 조명 업데이트
		UpdateLight(deltaTime);

		// 공용 Constant buffer 업데이트
		// DirectX는 Row-Major인데 HLSL는 Column-Major이므로 Transpose
		m_globalConstsCPU.view = m_mainCamera->GetViewMatrix().Transpose();
		m_globalConstsCPU.proj = m_mainCamera->GetProjMatrix().Transpose();
		m_globalConstsCPU.viewProj = m_globalConstsCPU.proj * m_globalConstsCPU.view; // Transpose 시켰으므로 곱셈 순서 주의
		m_globalConstsCPU.eyeWorld = m_mainCamera->GetPos();
		D3D11Utils::UpdateBuffer(context, m_globalConstsCPU, m_globalConstsGPU);

		triangle->Update(context, deltaTime);
	}

	void Scene::Render(RenderBase& renderer) {
		ComPtr<ID3D11DeviceContext>& context = renderer.GetContext();

		// Shader들에서 공통으로 사용할 Constant Buffer, Sampler State, SRV 등을 설정
		setGlobals(context);

		triangle->Render(renderer);

		m_skybox->Render(renderer);
	}

	void Scene::UpdateLight(const float& deltaTime)
	{

	}

	void Scene::MoveForward(float axis)
	{
		TransformComponent* tr = m_mainCamera->GetComponent<TransformComponent>();
		Vector3 camPos, tempPos;
		if (tr) {
			if (axis) {
				static float speed = 10.f;
				Vector3 pos = tr->GetPos();
				pos += speed * axis * tr->GetForwardDir() * AppBase::GetDeltaTime();
				tr->SetPos(pos);

				camPos = tr->GetPos();
			}
			
		}

		tempPos = triangle->GetComponent<TransformComponent>()->GetPos();
	}
	void Scene::setGlobals(ComPtr<ID3D11DeviceContext>& context)
	{
		// Global Constants을 Shader에서 사용할 수 있도록 설정
		context->VSSetConstantBuffers(0, 1, m_globalConstsGPU.GetAddressOf());
		context->GSSetConstantBuffers(0, 1, m_globalConstsGPU.GetAddressOf());
		context->PSSetConstantBuffers(0, 1, m_globalConstsGPU.GetAddressOf());

		// Shader들에서 공통으로 사용하는 Sampler States
		context->VSSetSamplers(0, UINT(RenderBase::graphicsCommon.sampleStates.size()),
			RenderBase::graphicsCommon.sampleStates.data());
		context->PSSetSamplers(0, UINT(RenderBase::graphicsCommon.sampleStates.size()),
			RenderBase::graphicsCommon.sampleStates.data());

		// Shader들에서 공통으로 사용할 IBL용 Texture들 설정
		m_skybox->SetCommonSRVs(context);
	}
}