#pragma once
//
//#include "InputTypes.h"
//#include "InputAction.h"
#include "InputManager.h"

namespace DE {
	class SampleActor;
	class CameraActor;
	class SkyboxActor;
	class RenderBase;
	class CopyFilter;
	class TreeBillboard;
	class Actor;
	class MirrorActor;
	class FogEffect;
	class SquareActor;
	class LightActor;

	class Scene
	{
	public:
		Scene();
		virtual ~Scene() {}
		virtual void Initialize();
		virtual void Update(const float& deltaTime);
		virtual void Render();

		CameraActor* GetMainCamera() { return m_mainCamera.get(); };

		uint8_t* GetPickColor() { return m_pickColor; }
	protected:
		virtual void UpdateLight(const float& deltaTime);
		virtual void SetGlobals(const ComPtr<ID3D11Buffer>& globalConstsGPU);
		virtual void UpdateGlobalConstants(const float& deltaTime, const Vector3& eyeWorld, const Matrix& view, const Matrix& proj);
		// Rendering
		virtual void RenderOpaqueObjects(); // 불투명한 물체 렌더링
		virtual void RenderMirror(); // 거울 렌더링
		// Depth값만 추출하기 위한 Depth Only Pass
		virtual void RenderDepthOnly();
		// 그림자를 위한 그림자 맵
		virtual void RenderShadowMap();
	private:
		void enableCamFpv();
		void pickingRay(float click);
		// Ray와 충돌한 가장 가까운 Actor
		Actor* pickClosest(const DirectX::SimpleMath::Ray& pickingRay, float& minDist);
		void pickingGpu(float click);
	protected:
		// Shader에서 공통으로 사용되는 Constant Buffer Data
		GlobalConstants m_globalConstsCPU;
		ComPtr<ID3D11Buffer> m_globalConstsGPU;

		std::shared_ptr<CameraActor> m_mainCamera;
		InputButton f = InputButton::F;
		InputAction m_fpv;
		InputButton lButton = InputButton::LButton;
		InputButton rButton = InputButton::RButton;
		InputAxis xAxis = InputAxis::XAxis;
		InputAxisAction m_mouseClick;

	private:
		std::shared_ptr<SampleActor> triangle;
		std::shared_ptr<SquareActor> ground;
		std::shared_ptr<SkyboxActor> m_skybox;
		std::shared_ptr<TreeBillboard> m_billboard;
		// 거울 반사
		std::shared_ptr<MirrorActor> m_mirror;

		std::shared_ptr<CopyFilter> m_copyPostProcess;
		std::shared_ptr<FogEffect> m_depthPP;
		
		// 0 row는 일반 actor, 1 row는 billboard
		std::vector<std::shared_ptr<Actor>> m_actorList[2];
		Actor* m_pickedActor = nullptr;

		uint8_t m_pickColor[4] = { 0, 0, 0, 0 };

		std::array<std::shared_ptr<LightActor>, MAX_LIGHTS> m_lights;
	};
}
