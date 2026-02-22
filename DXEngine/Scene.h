#pragma once

#include "InputManager.h"
#include "Gui.h"
#include "EffectActor.h"
#include "TransformComponent.h"

namespace DE {
	class SampleActor;
	class CameraActor;
	class SkyboxActor;
	class RenderBase;
	class BloomEffect;
	class TreeBillboard;
	class Actor;
	class MirrorActor;
	class FogEffect;
	class Gui;

	class Scene
	{
	public:
		enum class ActorCategory : uint8_t
		{
			Normal = 0,
			Billboard = 1,
			Effect = 2,
			Count = 3
		};

	public:
		Scene();
		virtual ~Scene();
		virtual void Initialize();
		virtual void Update(const float& deltaTime);
		virtual void UpdateGUI();
		virtual void Render();

		CameraActor* GetMainCamera() { return m_mainCamera.get(); }

		//  EffectActor ߰
		template<class T = EffectActor>
		T* SpawnEffect(const std::wstring& name, const std::wstring& presetPath, const Vector3& worldPos);
		EffectActor* SpawnEffect(std::unique_ptr<EffectActor> actor); //  ߰
		std::vector<std::unique_ptr<Actor>>& GetActorList(ActorCategory category);
		bool ContainsEffect(EffectActor* effect) const;

		void RemoveEffects(const std::vector<EffectActor*>& effectsToRemove);
	protected:
		// Actor Management
		template<class T>
		T* AddObject(const std::wstring& name);

		template<class T>
		T* AddEffect(const std::wstring& name);

		template<class T>
		T* AddLight(const std::wstring& name);

		template<class T>
		T* AddGui();

		// Initialization
		virtual void InitializeCommonResources();
		virtual void InitializeLights();
		virtual void InitializeCamera();
		virtual void InitializeSkybox();
		virtual void InitializeInput();

		// Update
		virtual void UpdateCamera(const float& deltaTime);
		virtual void UpdateLights(const float& deltaTime);
		virtual void UpdateActors(const float& deltaTime);
		virtual void UpdateGlobalConstants(const float& deltaTime, const Vector3& eyeWorld, const Matrix& view, const Matrix& proj);

		// Rendering
		virtual void SetGlobals(const ComPtr<ID3D11Buffer>& globalConstsGPU);
		virtual void SetCommonRenderStates();
		virtual void RenderDepthOnly();
		virtual void RenderShadowMaps();
		virtual void RenderOpaqueObjects();
		virtual void RenderActors(ActorCategory category);
		virtual void RenderDebugGeometry();

		// Effect
		virtual void CleanupFinishedEffects();
	private:
		void EnableCameraFPV();

	protected:
		// Rendering Constants
		GlobalConstants m_globalConstsCPU;
		ComPtr<ID3D11Buffer> m_globalConstsGPU;

		// [Added] CPU-side View/Proj for Frustum Culling
		Matrix m_view;
		Matrix m_proj;

		// Core Scene Elements
		std::unique_ptr<CameraActor> m_mainCamera;
		std::unique_ptr<SkyboxActor> m_skybox;
		std::unique_ptr<BloomEffect> m_bloomPostProcess;

		// Actors organized by category
		std::vector<std::unique_ptr<Actor>> m_actorList[static_cast<size_t>(ActorCategory::Count)];
		std::vector<std::unique_ptr<Actor>> m_lights;
		std::vector<std::unique_ptr<Gui>> m_guis;

		// Input - ư  
		InputButton m_fButton = InputButton::F;
		InputButton m_lButton = InputButton::LButton;
		InputButton m_rButton = InputButton::RButton;
		InputAxis m_xAxis = InputAxis::XAxis;
		
		// InputAction ߿  (ư )
		InputAction m_fpv;
		InputAxisAction m_mouseClick;
	};

	template<class T>
	inline T* Scene::AddObject(const std::wstring& name)
	{
		auto actor = std::make_unique<T>(name);
		T* rawPtr = actor.get();
		auto category = static_cast<size_t>(ActorCategory::Normal);
		m_actorList[category].emplace_back(std::move(actor));
		return rawPtr;
	}

	template<class T>
	inline T* Scene::AddEffect(const std::wstring& name)
	{
		auto actor = std::make_unique<T>(name);
		T* rawPtr = actor.get();
		auto category = static_cast<size_t>(ActorCategory::Effect);
		m_actorList[category].emplace_back(std::move(actor));
		return rawPtr;
	}

	template<class T>
	inline T* Scene::SpawnEffect(const std::wstring& name, const std::wstring& presetPath, const Vector3& worldPos)
	{
		auto actor = std::make_unique<T>(name);
		T* rawPtr = actor.get();

		if (auto* tr = actor->GetComponent<TransformComponent>())
			tr->SetPos(worldPos);

		actor->Initialize();

		if (actor->NeedsExternalPreset() && !presetPath.empty())
			actor->SetParticlePreset(presetPath);

		auto category = static_cast<size_t>(ActorCategory::Effect);
		m_actorList[category].emplace_back(std::move(actor));
		return rawPtr;
	}

	template<class T>
	inline T* Scene::AddLight(const std::wstring& name)
	{
		assert(m_lights.size() < MAX_LIGHTS && "Cannot add more lights than MAX_LIGHTS");
			
		std::unique_ptr<T> light = std::make_unique<T>(name);
		m_lights.emplace_back(std::move(light));
		return dynamic_cast<T*>(m_lights.back().get());
	}

	template<class T>
	inline T* Scene::AddGui()
	{
		auto gui = std::make_unique<T>();
		T* rawPtr = gui.get();
		m_guis.emplace_back(std::move(gui));
		return rawPtr;
	}
}
