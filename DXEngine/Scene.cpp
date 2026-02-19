#include "pch.h"
#include "Scene.h"

#include "CameraActor.h"
#include "SkyboxActor.h"
#include "TransformComponent.h"

#include "AppBase.h"
#include "InputManager.h"
#include "RenderBase.h"

#include "CopyFilter.h"
#include "FogEffect.h"

#include "SpotLight.h"
#include "PointLight.h"
#include "TextureManager.h"
#include "MaterialSystem.h"
#include "ParticleManager.h"
#include "ModelManager.h"

namespace DE {

	// Constants
	namespace SceneConstants {
		static constexpr Vector3 DEFAULT_CAMERA_POSITION = Vector3(0.f, 0.f, -2.f);
		static constexpr size_t DEFAULT_LIGHT_INDEX = 0;
	}

	Scene::Scene() 
		: m_xAxis(InputAxis::XAxis)
		, m_fpv(m_fButton)
		, m_mouseClick(m_lButton, m_rButton)
	{
		InitializeCommonResources();
	}

	Scene::~Scene()
	{
	}

	EffectActor* Scene::SpawnEffect(std::unique_ptr<EffectActor> actor)
	{
		if (!actor)
			return nullptr;

		EffectActor* rawPtr = actor.get();
		auto category = static_cast<size_t>(ActorCategory::Effect);
		m_actorList[category].emplace_back(std::move(actor));

		return rawPtr;
	}

	std::vector<std::unique_ptr<Actor>>& Scene::GetActorList(ActorCategory category)
	{
		return m_actorList[static_cast<size_t>(category)];
	}

	bool Scene::ContainsEffect(EffectActor* effect) const
	{
		for (const auto& e : m_actorList[static_cast<size_t>(ActorCategory::Effect)]) {
			if (e.get() == effect) return true;
		}
		return false;
	}

	void Scene::RemoveEffects(const std::vector<EffectActor*>& effectsToRemove)
	{
		if (effectsToRemove.empty()) return;

		// unordered_set으로 O(1) 조회 최적화
		std::unordered_set<EffectActor*> toRemove(effectsToRemove.begin(), effectsToRemove.end());

		auto& effectList = m_actorList[static_cast<size_t>(ActorCategory::Effect)];

		// 한 번의 순회로 모든 항목 제거 (O(N))
		// 개별 삭제 O(N*M) -> 배치 삭제 O(N+M)
		std::erase_if(effectList,
			[&toRemove](const std::unique_ptr<Actor>& actor) {
				EffectActor* effect = dynamic_cast<EffectActor*>(actor.get());
				return effect && toRemove.count(effect) > 0;
			});
	}

	void Scene::InitializeCommonResources()
	{
		RenderBase& renderer = *GET_SINGLE(RenderBase);
		ComPtr<ID3D11Device>& device = renderer.GetDevice();

		// Create common constant buffer
		D3D11Utils::CreateConstantBuffer(device.Get(), m_globalConstsCPU, m_globalConstsGPU);
		
		// Initialize managers
		TextureManager::Get().Initialize();
		MaterialSystem::Get().Initialize();
		ModelManager::Get().Initialize();
		ParticleManager::Get().Initialize();

		// Create core actors
		m_mainCamera = std::make_unique<CameraActor>(L"MainCamera");
		m_skybox = std::make_unique<SkyboxActor>(L"Skybox");

		// Setup post-processing
		m_copyPostProcess = std::make_unique<CopyFilter>();
		renderer.SetPostProcess(*m_copyPostProcess.get(), RenderBase::graphicsCommon.postProcess.basicPSO);
	}

	void Scene::Initialize()
	{
		InitializeLights();
		InitializeCamera();
		InitializeSkybox();
		InitializeInput();

		// Initialize all actors
		for (auto& actorList : m_actorList)
		{
			for (auto& actor : actorList)
			{
				actor->Initialize();
			}
		}

		// Initialize GUIs
		for (auto& gui : m_guis)
		{
			gui->Initialize();
		}
	}

	void Scene::InitializeLights()
	{
		std::vector<LightActor*> lights;
		lights.reserve(MAX_LIGHTS);

		// Create temp lights up to MAX_LIGHTS
		for (size_t i = m_lights.size(); i < MAX_LIGHTS; ++i)
		{
			auto tempLight = std::make_unique<SpotLight>(L"TempLight");
			tempLight->TurnOff();
			m_lights.emplace_back(std::move(tempLight));
			lights.emplace_back(dynamic_cast<LightActor*>(m_lights.back().get()));
		}

		// Turn on first light by default
		lights[SceneConstants::DEFAULT_LIGHT_INDEX]->TurnOn();

		// Create shadow array buffer
		GET_SINGLE(RenderBase)->CreateShadowArrayBuffer(lights);

		// Initialize all lights
		for (auto& light : m_lights)
		{
			light->Initialize();
		}
	}

	void Scene::InitializeCamera()
	{
		m_mainCamera->Initialize();
		
		if (TransformComponent* transform = m_mainCamera->GetComponent<TransformComponent>())
		{
			transform->SetPos(SceneConstants::DEFAULT_CAMERA_POSITION);
		}
	}

	void Scene::InitializeSkybox()
	{
		m_skybox->Initialize();
	}

	void Scene::InitializeInput()
	{
		// F Ű  ī޶ FPV  
		AppBase::GetInputManager().BindInputAction(m_fpv, InputState::Pressed, this, &Scene::EnableCameraFPV);
	}

	void Scene::Update(const float& deltaTime)
	{
		UpdateGUI();
		UpdateCamera(deltaTime);

		const Vector3 eyeWorld = m_mainCamera->GetPos();
		m_view = m_mainCamera->GetViewMatrix();
		m_proj = m_mainCamera->GetProjMatrix();

		UpdateGlobalConstants(deltaTime, eyeWorld, m_view, m_proj);
		UpdateLights(deltaTime);
		UpdateActors(deltaTime);

		//  ߰ EffectActor Ϸ ͵ 
		CleanupFinishedEffects();

		// ParticleManager handles all particle updates
		ParticleManager::Get().Update(deltaTime);
	}

	void Scene::UpdateCamera(const float& deltaTime)
	{
		m_mainCamera->Update(deltaTime);
	}

	void Scene::UpdateLights(const float& deltaTime)
	{
		for (size_t i = 0; i < m_lights.size(); ++i)
		{
			m_lights[i]->Update(deltaTime);
			
			// Assuming m_lights contains only LightActor derived types
			if (LightActor* lightActor = dynamic_cast<LightActor*>(m_lights[i].get()))
			{
				m_globalConstsCPU.lights[i] = lightActor->GetLight();
			}
		}
	}

	void Scene::UpdateActors(const float& deltaTime)
	{
		for (auto& actorList : m_actorList)
		{
			for (auto& actor : actorList)
			{
				actor->Update(deltaTime);
			}
		}
	}

	void Scene::UpdateGlobalConstants(const float& deltaTime, const Vector3& eyeWorld, const Matrix& view, const Matrix& proj)
	{
		RenderBase& renderer = *GET_SINGLE(RenderBase);
		
		// DirectX uses Row-Major but HLSL uses Column-Major, so transpose
		const Matrix transposedView = view.Transpose();
		const Matrix transposedProj = proj.Transpose();
		
		m_globalConstsCPU.view = transposedView;
		m_globalConstsCPU.proj = transposedProj;
		m_globalConstsCPU.viewProj = transposedProj * transposedView;
		m_globalConstsCPU.invProj = proj.Invert().Transpose();
		m_globalConstsCPU.invViewProj = m_globalConstsCPU.viewProj.Invert();
		m_globalConstsCPU.eyeWorld = eyeWorld;
		
		D3D11Utils::UpdateBuffer(renderer.GetContext(), m_globalConstsCPU, m_globalConstsGPU);
	}

	void Scene::UpdateGUI()
	{
		GuiBase* guiBase = GET_SINGLE(GuiBase);
		guiBase->PreUpdate();

		for (auto& gui : m_guis)
		{
			gui->Update();
		}
	}

	void Scene::Render()
	{
		SetGlobals(m_globalConstsGPU);
		SetCommonRenderStates();

		RenderDepthOnly();
		RenderShadowMaps();

		// Set common IBL textures
		//m_skybox->SetCommonSRVs();

		// Render opaque objects
		RenderOpaqueObjects();
	}

	void Scene::SetGlobals(const ComPtr<ID3D11Buffer>& globalConstsGPU)
	{
		RenderBase& renderer = *GET_SINGLE(RenderBase);
		ComPtr<ID3D11DeviceContext>& context = renderer.GetContext();
		
		ID3D11Buffer* const buffers[] = { globalConstsGPU.Get() };
		
		// Set global constants for all shader stages
		context->VSSetConstantBuffers(0, 1, buffers);
		context->GSSetConstantBuffers(0, 1, buffers);
		context->PSSetConstantBuffers(0, 1, buffers);
	}

	void Scene::SetCommonRenderStates()
	{
		RenderBase& renderer = *GET_SINGLE(RenderBase);
		ComPtr<ID3D11DeviceContext>& context = renderer.GetContext();

		const UINT samplerCount = static_cast<UINT>(RenderBase::graphicsCommon.sampleStates.size());
		ID3D11SamplerState* const* samplers = RenderBase::graphicsCommon.sampleStates.data();

		// Set common sampler states for all shader stages
		context->VSSetSamplers(0, samplerCount, samplers);
		context->PSSetSamplers(0, samplerCount, samplers);
		context->CSSetSamplers(0, samplerCount, samplers);
	}

	void Scene::RenderDepthOnly()
	{
		RenderBase& renderer = *GET_SINGLE(RenderBase);
		renderer.SetDepthOnlyRender();
		renderer.SetPipelineState(RenderBase::graphicsCommon.depth.depthOnlyPSO);

		RenderActors(ActorCategory::Normal);
		RenderActors(ActorCategory::Billboard);
		RenderActors(ActorCategory::Effect);
		//ParticleManager::Get().RenderDepth();
		//m_skybox->Render();
	}

	void Scene::RenderShadowMaps()
	{
		RenderBase& renderer = *GET_SINGLE(RenderBase);

		ID3D11ShaderResourceView* nullSRV = nullptr;
		renderer.GetContext()->PSSetShaderResources(15, 1, &nullSRV);
		for (auto& light : m_lights)
			if (LightActor* lightActor = dynamic_cast<LightActor*>(light.get()))
				if (lightActor->GetLight().type & LIGHT_SHADOW)
					lightActor->RenderShadow(m_actorList, static_cast<size_t>(ActorCategory::Count));
	}

	void Scene::RenderOpaqueObjects()
	{
		RenderBase& renderer = *GET_SINGLE(RenderBase);
		
		renderer.SetViewport();
		renderer.SetRender();

		SetGlobals(m_globalConstsGPU);
		// Set shadow maps as SRVs
		renderer.SetShadowSRVs();

		// Render normal objects
		renderer.SetPipelineState(RenderBase::graphicsCommon.basic.solidPSO);
		RenderActors(ActorCategory::Normal);
		RenderActors(ActorCategory::Effect);

		// Render billboards
		renderer.SetPipelineState(RenderBase::graphicsCommon.billboard.solidPSO);
		RenderActors(ActorCategory::Billboard);

		// Render skybox
		//renderer.SetPipelineState(RenderBase::graphicsCommon.skybox.solidPSO);
		//m_skybox->Render();

		// Render debug geometry
		RenderDebugGeometry();

		// Render particles (ParticleManager handles PSO restoration)
		// CPU Frustum Culling용으로 Update()에서 저장한 View, Proj 전달
		ParticleManager::Get().SetViewAndProj(m_view, m_proj);
		ParticleManager::Get().Render();
	}

	void Scene::RenderActors(ActorCategory category)
	{
		const auto& actorList = m_actorList[static_cast<size_t>(category)];
		for (const auto& actor : actorList)
		{
			actor->Render();
		}
	}

	void Scene::RenderDebugGeometry()
	{
		RenderBase& renderer = *GET_SINGLE(RenderBase);

		// Render bounding volumes
		renderer.SetPipelineState(RenderBase::graphicsCommon.basic.boundPSO);
		for (const auto& actorList : m_actorList)
		{
			for (const auto& actor : actorList)
			{
				actor->RenderBoundingVolume();
			}
		}

		// Render normals
		renderer.SetPipelineState(RenderBase::graphicsCommon.normal.solidPSO);
		for (const auto& actorList : m_actorList)
		{
			for (const auto& actor : actorList)
			{
				actor->RenderNormal();
			}
		}
	}

	void Scene::CleanupFinishedEffects()
	{
		auto& effectList = m_actorList[static_cast<size_t>(ActorCategory::Effect)];

		// Ϸ ͵ 
		std::erase_if(effectList,
			[](const std::unique_ptr<Actor>& actor) {
				if (auto* effect = dynamic_cast<EffectActor*>(actor.get())) {
					return effect->IsFinished();
				}
				return false;
			});
	}

	void Scene::EnableCameraFPV()
	{
		m_mainCamera->EnableFPV();
	}
}