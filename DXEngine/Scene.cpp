#include "pch.h"
#include "Scene.h"
#include "CameraActor.h"
#include "TransformComponent.h"

#include "AppBase.h"
#include "InputManager.h"

#include "TriangleActor.h"

namespace DE {
	Scene::Scene(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context) : m_device(device), m_context(context)
	{
		// �������� ���̴� Constant buffer
		D3D11Utils::CreateConstantBuffer(m_device, m_globalConstsCPU, m_globalConstsGPU);

		m_mainCamera = std::make_shared<CameraActor>(L"MainCamera");

		action = InputAxisAction(w, s);

		triangle = std::make_shared<TriangleActor>(L"Temp");
	}

	void Scene::Initialize() {
		// ���� ����
		{
			// ���� ������ �ִ� ���� 3��
			// Spot Light
			m_globalConstsCPU.lights[0].radiance = Vector3(1.0f);
			m_globalConstsCPU.lights[0].position = Vector3(0.0f, 0.0f, -2.0f);  // ������ �񽺵���
			m_globalConstsCPU.lights[0].direction = Vector3(0.0f, 0.0f, 1.0f);  // �Ʒ� ��������
			m_globalConstsCPU.lights[0].spotPower = 10.0f;                      // �� �� ���ߵ� ��
			m_globalConstsCPU.lights[0].fallOffStart = 0.0f;
			m_globalConstsCPU.lights[0].fallOffEnd = 10.0f;
			m_globalConstsCPU.lights[0].radius = 0.f;
			m_globalConstsCPU.lights[0].type = LIGHT_SPOT;

			// ���� 1���� ����ϰ� �������� ������� ����
			m_globalConstsCPU.lights[1].type = LIGHT_OFF;
			m_globalConstsCPU.lights[2].type = LIGHT_OFF;
		}

		// ���� ��ġ ǥ��
		{
			// ������ �� ���� ���°� �ִ� ������ ȭ�鿡 �������Ҷ� ����� Actor
			for (int i = 0; i < MAX_LIGHTS; ++i) {
				
			}
		}

		// ī�޶� ��ġ ǥ��
		{
			TransformComponent* tr = m_mainCamera->GetComponent<TransformComponent>();
			if (tr) {
				tr->SetPos(Vector3(0.f, 0.f, -2.f));
			}
		}

		// �Է� Bind
		{
			AppBase::GetInputManager().BindInputAxis(axis, action, this, &Scene::MoveForward);
		}

		triangle->Initialize(m_device);
		TransformComponent* tr = triangle->GetComponent<TransformComponent>();
		if (tr) {
			tr->SetScale(Vector3(0.5f));
		}

	}

	void Scene::Update(const float& deltaTime) {
		// ���� ������Ʈ
		UpdateLight(deltaTime);

		// ���� Constant buffer ������Ʈ
		// DirectX�� Row-Major�ε� HLSL�� Column-Major�̹Ƿ� Transpose
		m_globalConstsCPU.view = m_mainCamera->GetViewMatrix().Transpose();
		m_globalConstsCPU.proj = m_mainCamera->GetProjMatrix().Transpose();
		m_globalConstsCPU.eyeWorld = m_mainCamera->GetPos();
		D3D11Utils::UpdateBuffer(m_context, m_globalConstsCPU, m_globalConstsGPU);

		triangle->Update(m_context, deltaTime);
	}

	void Scene::Render() {
		// Global Constants�� Shader���� ����� �� �ֵ��� ����
		SetGlobalConsts();

		triangle->Render(m_context);
	}

	void Scene::SetGlobalConsts()
	{
		m_context->VSSetConstantBuffers(0, 1, m_globalConstsGPU.GetAddressOf());
		m_context->PSSetConstantBuffers(0, 1, m_globalConstsGPU.GetAddressOf());
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
}