#include "pch.h"
#include "Scene.h"

#include "CameraActor.h"
#include "SkyboxActor.h"
#include "TransformComponent.h"

#include "AppBase.h"
#include "InputManager.h"
#include "RenderBase.h"

#include "CopyFilter.h"
#include "BoundComponent.h"
#include "TreeBillboard.h"
#include "MirrorActor.h"
#include "FogEffect.h"

#include "SpotLight.h"
#include "PointLight.h"

namespace DE {
	Scene::Scene() : xAxis(InputAxis::XAxis)
	{
		RenderBase& renderer = *GET_SINGLE(RenderBase);
		ComPtr<ID3D11Device>& device = renderer.GetDevice();
		ComPtr<ID3D11DeviceContext>& context = renderer.GetContext();

		// 공통으로 쓰이는 Constant buffer
		D3D11Utils::CreateConstantBuffer(device.Get(), m_globalConstsCPU, m_globalConstsGPU);

		// Scene 공통 Actor
		{
			m_mainCamera = std::make_shared<CameraActor>(L"MainCamera");
			//m_actorList.emplace_back(m_mainCamera);
			m_fpv = InputAction(f);

			m_skybox = std::make_shared<SkyboxActor>(L"Skybox");
			//m_actorList.emplace_back(m_skybox);

			m_mouseClick = InputAxisAction(lButton, rButton);
		}

		m_copyPostProcess = std::make_shared<CopyFilter>();
		renderer.SetPostProcess(*m_copyPostProcess.get(), RenderBase::graphicsCommon.postProcess.basicPSO);

		//m_depthPP = std::make_shared<FogEffect>();
		//renderer.SetPostProcess(*m_depthPP.get(), RenderBase::graphicsCommon.postProcess.basicPSO);

		//m_billboard = std::make_shared<TreeBillboard>(L"trees");
		//m_actorList[1].emplace_back(m_billboard);

		//m_mirror = std::make_shared<MirrorActor>(L"Mirror");
		//m_mirror->SetVisible(false);
	}

	void Scene::Initialize() {
		// 조명 설정
		{
			std::vector<LightActor*> lights;
			for (size_t i = m_lights.size(); i < MAX_LIGHTS; ++i) {
				std::unique_ptr<PointLight> tempLight = std::make_unique<PointLight>(L"TempLight");
				tempLight->TurnOff();
				m_lights.emplace_back(std::move(tempLight));
				lights.emplace_back(dynamic_cast<LightActor*>(m_lights.back().get()));
			}
			lights[0]->TurnOn();

			GET_SINGLE(RenderBase)->CreateShadowArrayBuffer(lights);
			// IBL은 그림자를 구현하지 않고 AO를 사용해 그림자 효과를 비슷하게 구현함
			// (TODO: Direct Light으로 자연광 효괄르 구현하는게 좋을듯)
			for (int i = 0; i < MAX_LIGHTS; ++i)
				m_lights[i]->Initialize();

			//auto* tr = m_lights[0]->GetComponent<TransformComponent>();
			//if (tr) {
			//	Vector3 pos = tr->GetPos();
			//	pos = Vector3::Transform(pos, Matrix::CreateRotationY(deltaTime * 0.5f));
			//	tr->SetPos(pos);
			//}

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
			//AppBase::GetInputManager().BindInputAxis(xAxis, m_mouseClick, this, &Scene::pickingRay);
		}

		for (auto& actorList : m_actorList)
			for (auto& actor : actorList)
				actor->Initialize();

		//tr = m_billboard->GetComponent<TransformComponent>();
		//if (tr) {
		//	tr->SetPos(Vector3(0.f, 0.f, 5.f));
		//}

		//m_mirror->Initialize();
		//tr = m_mirror->GetComponent<TransformComponent>();
		//if (tr) {
		//	//tr->SetPos({ 0.f, -0.5f, -.5f });
		//	//tr->SetRotation(0.f, 90.f, 0.f);
		//}

		for (auto& gui : m_guis)
			gui->Initialize();
	}

	void Scene::Update(const float& deltaTime) {
		RenderBase& renderer = *GET_SINGLE(RenderBase);
		ComPtr<ID3D11DeviceContext>& context = renderer.GetContext();

		UpdateGUI();

		// Camera Update
		m_mainCamera->Update(deltaTime);

		const Vector3 eyeWorld = m_mainCamera->GetPos();
		const Matrix view = m_mainCamera->GetViewMatrix();
		const Matrix proj = m_mainCamera->GetProjMatrix();

		// 공용 Constant buffer 업데이트
		UpdateGlobalConstants(deltaTime, eyeWorld, view, proj);
		// 조명 업데이트
		UpdateLight(deltaTime);

		for (auto& actorList : m_actorList)
			for (auto& actor : actorList)
				actor->Update(deltaTime);
		
		//m_mirror->Update(deltaTime);
		//m_mirror->UpdateGlobalConstants(m_globalConstsCPU, deltaTime, eyeWorld, view, proj);
		
		// TODO: Picking Test
		//pickingGpu(0);
	}

	void Scene::Render() {
		RenderBase& renderer = *GET_SINGLE(RenderBase);
		ComPtr<ID3D11DeviceContext>& context = renderer.GetContext();
		// Shader들에서 공통으로 사용할 Constant Buffer, Sampler State등을 설정
		SetGlobals(m_globalConstsGPU);

		// Shader들에서 공통으로 사용하는 Sampler States
		context->VSSetSamplers(0, UINT(RenderBase::graphicsCommon.sampleStates.size()),
			RenderBase::graphicsCommon.sampleStates.data());
		context->PSSetSamplers(0, UINT(RenderBase::graphicsCommon.sampleStates.size()),
			RenderBase::graphicsCommon.sampleStates.data());

		RenderDepthOnly();
		RenderShadowMap();

		// Shader들에서 공통으로 사용할 IBL용 Texture들 설정
		m_skybox->SetCommonSRVs();

		// 불투명 물체들 렌더링
		RenderOpaqueObjects();

		// 거울 렌더링
		RenderMirror();
	}

	void Scene::UpdateLight(const float& deltaTime)
	{
		//auto* tr = m_lights[0]->GetComponent<TransformComponent>();
		//if (tr) {
		//	Vector3 pos = tr->GetPos();
		//	pos = Vector3::Transform(pos, Matrix::CreateRotationY(deltaTime * 0.5f));
		//	tr->SetPos(pos);
		//}

		LightActor* light;
		// 그림자맵을 만들기 위한 시점
		for (int i = 0; i < MAX_LIGHTS; ++i) {
			light = dynamic_cast<LightActor*>(m_lights[i].get());
			light->Update(deltaTime);

			m_globalConstsCPU.lights[i] = light->GetLight();
		}
	}

	void Scene::SetGlobals(const ComPtr<ID3D11Buffer>& globalConstsGPU)
	{
		RenderBase& renderer = *GET_SINGLE(RenderBase);
		ComPtr<ID3D11DeviceContext>& context = renderer.GetContext();
		// Global Constants을 Shader에서 사용할 수 있도록 설정
		context->VSSetConstantBuffers(0, 1, globalConstsGPU.GetAddressOf());
		context->GSSetConstantBuffers(0, 1, globalConstsGPU.GetAddressOf());
		context->PSSetConstantBuffers(0, 1, globalConstsGPU.GetAddressOf());
	}

	void Scene::UpdateGlobalConstants(const float& deltaTime, const Vector3& eyeWorld, const Matrix& view, const Matrix& proj)
	{
		RenderBase& renderer = *GET_SINGLE(RenderBase);
		// DirectX는 Row-Major인데 HLSL는 Column-Major이므로 Transpose
		m_globalConstsCPU.view = view.Transpose();
		m_globalConstsCPU.proj = proj.Transpose();
		m_globalConstsCPU.viewProj = m_globalConstsCPU.proj * m_globalConstsCPU.view; // Transpose 시켰으므로 곱셈 순서 주의
		m_globalConstsCPU.invProj = proj.Invert().Transpose();
		m_globalConstsCPU.invViewProj = m_globalConstsCPU.viewProj.Invert(); // 그림자 렌더링에 사용
		m_globalConstsCPU.eyeWorld = eyeWorld;
		D3D11Utils::UpdateBuffer(renderer.GetContext(), m_globalConstsCPU, m_globalConstsGPU);
	}

	void Scene::UpdateGUI()
	{
		GuiBase* guiBase = GET_SINGLE(GuiBase);
		guiBase->PreUpdate();

		for (auto& gui : m_guis)
			gui->Update();
	}

	void Scene::RenderOpaqueObjects()
	{
		RenderBase& renderer = *GET_SINGLE(RenderBase);
		// 원래 렌더링 해상도
		renderer.SetViewport();
		renderer.SetRender();

		SetGlobals(m_globalConstsGPU);

		// 그림자맵들도 공용 Texture들 이후에 추가
		// 주의: 마지막 shadowDSV를 RenderTarget에서 해제한 후 설정
		renderer.SetShadowSRVs();

		// 거울 없이 렌더링
		renderer.SetPipelineState(RenderBase::graphicsCommon.basic.solidPSO);
		for (auto& actor : m_actorList[0])
			actor->Render();

		renderer.SetPipelineState(RenderBase::graphicsCommon.billboard.solidPSO);
		for (auto& billboard : m_actorList[1])
			billboard->Render();

		renderer.SetPipelineState(RenderBase::graphicsCommon.skybox.solidPSO);
		m_skybox->Render();

		// Bounding Volume 그리기
		renderer.SetPipelineState(RenderBase::graphicsCommon.basic.boundPSO);
		for (auto& actor : m_actorList[0])
			actor->RenderBoundingVolume();

		for (auto& billboard : m_actorList[1])
			billboard->RenderBoundingVolume();

		// Normal 그리기
		renderer.SetPipelineState(RenderBase::graphicsCommon.normal.solidPSO);
		for (auto& actor : m_actorList[0])
			actor->RenderNormal();

		for (auto& billboard : m_actorList[1])
			billboard->RenderNormal();
	}

	void Scene::RenderMirror()
	{
		// 거울 렌더링
		//m_mirror->Render(m_actorList, m_skybox, m_globalConstsGPU);
	}

	void Scene::RenderDepthOnly()
	{
		RenderBase& renderer = *GET_SINGLE(RenderBase);
		renderer.SetDepthOnlyRender();
		// 전부 렌더링
		renderer.SetPipelineState(RenderBase::graphicsCommon.depth.depthOnlyPSO);

		for (auto& actor : m_actorList[0])
			actor->Render();
		
		//m_mirror->Render(); // 거울만 렌더링

		for (auto& billboard : m_actorList[1])
			billboard->Render();
		
		m_skybox->Render();
	}

	void Scene::RenderShadowMap()
	{
		RenderBase& renderer = *GET_SINGLE(RenderBase);
		// RenderShadowMap()전에 RenderDepthOnly()를 실행시켜서 PSO가 depthOnly로 설정되어 있는 상태
		// 그림자맵 만들기
		//renderer.SetShadowViewport(); // 그림자맵 해상도

		ComPtr<ID3D11DeviceContext>& context = renderer.GetContext();

		LightActor* light;
		for (int i = 0; i < MAX_LIGHTS; i++) {
			light = dynamic_cast<LightActor*>(m_lights[i].get());
			if (light->GetLight().type & LIGHT_SHADOW) {
				//renderer.SetShadowMapRender(m_lights[i]->GetLightID());
				
				//light->RenderShadow({ m_actorList[0], m_actorList[1], {m_mirror} });
				light->RenderShadow({ m_actorList[0], m_actorList[1] });
				//for (auto& actor : m_actorList)
				//	m_lights[i]->RenderShadow(actor);
				//m_lights[i]->RenderShadow({ m_mirror });
			}
		}
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
		for (auto& actor : m_actorList[0]) {
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

	void Scene::pickingGpu(float click)
	{
		// GPU -> CPU로 화면의 Pixel 캡쳐

		// TODO: Mouse Picking Test
		for (auto a : m_actorList[0]) {
			Actor* actor = a.get();
			if (actor && memcmp(actor->GetHashColor(), m_pickColor, 4) == 0) {
				std::wcout << actor->GetName() << std::endl;
			}
		}
	}
}