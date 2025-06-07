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
#include "BoundComponent.h"
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
			m_actorList.emplace_back(m_mainCamera);
			m_fpv = InputAction(f);

			m_skybox = std::make_shared<SkyboxActor>(device, L"Skybox");
			m_actorList.emplace_back(m_skybox);

			m_mouseClick = InputAxisAction(lButton, rButton);
		}

		triangle = std::make_shared<SampleActor>(device, L"Temp");
		m_actorList.emplace_back(triangle);

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
			AppBase::GetInputManager().BindInputAction(m_fpv, InputState::Pressed, this, &Scene::enableCamFpv);
			AppBase::GetInputManager().BindInputAxis(xAxis, m_mouseClick, this, &Scene::pickingRay);
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

		// Camera Update
		m_mainCamera->Update(context, deltaTime);

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

	void Scene::enableCamFpv()
	{
		m_mainCamera->EnableFPV();
	}

	void Scene::pickingRay(float click)
	{
		static Actor* activeActor = nullptr;
		static float prevRatio = 0.f;
		static Vector3 prevPos(0.f);
		static Vector3 prevVector(0.f);

		// 적용할 회전과 이동 초기화
		Quaternion q = Quaternion::CreateFromAxisAngle(Vector3(1.f, 0.f, 0.f), 0.f);
		Vector3 dragTranslation(0.f);
		Vector3 pickPoint(0.f);
		float dist = 0.f;

		// 사용자가 왼쪽 마우스 버튼(1.f), 오른쪽 마우스버튼(-1.f) 중 하나만 누른다고 가정
		if (click) {
			const Matrix viewRow = m_mainCamera->GetViewMatrix();
			const Matrix projRow = m_mainCamera->GetProjMatrix();
			const Matrix invProjView = (viewRow * projRow).Invert();

			const Vector2 mouseNdc = AppBase::GetInputManager().GetMouseNDC();
			const Vector3 ndcNear = Vector3(mouseNdc.x, mouseNdc.y, 0.f);
			const Vector3 ndcFar = Vector3(mouseNdc.x, mouseNdc.y, 1.f);

			// 역변환으로 NDC->World 좌표계 구하기
			const Vector3 worldNear = Vector3::Transform(ndcNear, invProjView);
			const Vector3 worldFar = Vector3::Transform(ndcFar, invProjView);

			// Ray를 쏠 방향
			Vector3 dir = worldFar - worldNear;
			dir.Normalize();

			// 마우스의 NDC 좌표로부터 월드 좌표계의 값을 계산해 월드 좌표계에서 Ray를 하나 쏴주기
			const DirectX::SimpleMath::Ray curRay = DirectX::SimpleMath::Ray(worldNear, dir);

			// 이전 프레임에서 아무 물체도 선택되지 않았을 경우에는 새로 선택
			if (!activeActor) {
				Actor* newActor = pickClosest(curRay, dist);
				if (newActor) {
					std::wcout << "New Actor Selected: " << newActor->GetName() << std::endl;
					activeActor = newActor;
					m_pickedActor = newActor;
					
					// Actor가 선택된 좌표
					pickPoint = curRay.position + dist * curRay.direction;
					// 왼쪽 마우스 버튼 클릭인 경우 (물체를 회전시킬 예정)
					if (click > 0) {
						BoundComponent* bound = activeActor->GetComponent<BoundComponent>();
						if (bound) {
							// 회전시킬 것이므로 선택된 Actor를 가르키는 방향(Direction)이 다르면 회전시켜주면 됨
							prevVector = pickPoint - bound->GetBoundingSphere().Center;
							prevVector.Normalize();
						}
					}
					// 오른쪽 마우스 버튼 클릭인 경우 (물체를 이동시킬 예정)
					else {
						// Actor까지의 거리와 WorldFar - WorldNear의 비율
						prevRatio = dist / (worldFar - worldNear).Length();
						prevPos = pickPoint;
					}
				}	
			}
			// 이미 선택된 물체가 있었던 경우
			else {
				// 왼쪽 마우스 버튼 클릭으로는 회전
				if (click > 0) {
					BoundComponent* bound = activeActor->GetComponent<BoundComponent>();
					if (!bound)
						return;

					if (curRay.Intersects(bound->GetBoundingSphere(), dist)) {
						pickPoint = curRay.position + dist * curRay.direction;
					}
					else {
						// Bounding Sphere에 가장 가까운 점을 찾기
						Vector3 c = bound->GetBoundingSphere().Center - worldNear; // 화면 중심에서 선택된 Actor의 중심까지의 벡터
						// 선택된 Actor의 중심(c)에서 Ray에 c 벡터를 Projection한 벡터(dir.Dot(c) * dir)로 향하는 벡터
						Vector3 centerToRay = dir.Dot(c) * dir - c; // 즉, 선택된 Actor의 중심에서 Ray까지 가장 짧은 벡터를 계산
						// clamp(...) = Actor와 Ray와의 거리가 너무 멀어질수록 값이 커지고 가까울수록 값이 작아지는데 범위 [0.0, 1.0]으로 설정
						// Actor는 부피가 있을 것이므로 c + centerToRay * ratio로 항상 Actor의 중심이 아닌 중심에서 떨어진 곳일 수 있음
						// ex) Bounding Sphere같이 원인 경우 Actor의 오른쪽을 pick하면 중심이 아닌 구의 오른쪽이 선택되어야 함
						pickPoint = c + centerToRay * std::clamp(bound->GetBoundingSphere().Radius / centerToRay.Length(), 0.f, 1.f);
						pickPoint += worldNear; // World 좌표계에서의 위치값으로 변환
					}

					Vector3 currentVector = pickPoint - bound->GetBoundingSphere().Center;
					currentVector.Normalize();
					float theta = std::acos(prevVector.Dot(currentVector));

					if (theta > DirectX::XM_PI / 180.f * 3.f) {
						Vector3 axis = prevVector.Cross(currentVector);
						axis.Normalize();
						q = Quaternion::CreateFromAxisAngle(axis, theta);

						prevVector = currentVector;
					}
				}
				// 오른쪽 마우스 버튼으로는 이동
				else {
					Vector3 newPos = worldNear + prevRatio * (worldFar - worldNear);
					if ((newPos - prevPos).Length() > 1e-3) {
						dragTranslation = newPos - prevPos;
						prevPos = newPos;
					}

					pickPoint = newPos;
				}
			}
		}
		else {
			// 버튼에서 손을 떼면 움직일 모델은 nullptr로 설정
			activeActor = nullptr;

			// m_pickedActor는 GUI 조작을 위해 마우스에서 손을 뗴도 nullptr로 설정하지 않음
		}

		if (activeActor) {
			//TransformComponent* tr = activeActor->GetComponent<TransformComponent>();
			//if (tr) {
			//	tr->Rotate(q);
			//	tr->Translate(dragTranslation);
			//}
		}
	}
	Actor* Scene::pickClosest(const DirectX::SimpleMath::Ray& pickingRay, float& minDist)
	{
		minDist = 1e5f;
		Actor* minActor = nullptr;
		for (auto& actor : m_actorList) {
			BoundComponent* bound = actor->GetComponent<BoundComponent>();
			// 선택 가능한 Actor인지 확인
			if (!bound || !bound->IsPickable())
				continue;

			float dist = 0.f;
			// Ray와 충돌했으며 선택 가능한 거리에 안에 있는지 확인
			if (pickingRay.Intersects(bound->GetBoundingSphere(), dist) && dist < minDist) {
				minActor = actor.get();
				minDist = dist;
			}
			
		}

		return minActor;
	}
}