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
	class BloomEffect;

	class Scene
	{
	public:
		Scene(RenderBase& renderer);
		virtual ~Scene() {}
		virtual void Initialize();
		virtual void Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime);
		virtual void Render(RenderBase& renderer);

		CameraActor* GetMainCamera() { return m_mainCamera.get(); };

	protected:
		virtual void UpdateLight(const float& deltaTime);
		virtual void setGlobals(ComPtr<ID3D11DeviceContext>& context);

	private:
		void MoveForward(float axis);

	protected:
		std::shared_ptr<CameraActor> m_mainCamera;

		// Shader에서 공통으로 사용되는 Constant Buffer Data
		GlobalConstants m_globalConstsCPU;
		ComPtr<ID3D11Buffer> m_globalConstsGPU;
		
		// Main Camera 용 Input
		InputAxis axis = InputAxis::ZAxis;
		InputButton w = InputButton::W, s = InputButton::S;
		InputAxisAction action;

	private:
		std::shared_ptr<SampleActor> triangle;

		std::shared_ptr<SkyboxActor> m_skybox;

		std::shared_ptr<BloomEffect> m_blommPostProcess;

	};
}
