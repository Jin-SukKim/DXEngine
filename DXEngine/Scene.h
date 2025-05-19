#pragma once
//
//#include "InputTypes.h"
//#include "InputAction.h"
#include "InputManager.h"

namespace DE {
	class TriangleActor;
	class CameraActor;

	class Scene
	{
	public:
		Scene(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context);
		virtual ~Scene() {}
		virtual void Initialize();
		virtual void Update(const float& deltaTime);
		virtual void Render();
		void SetGlobalConsts();

		CameraActor* GetMainCamera() { return m_mainCamera.get(); };

		void MoveForward(float axis);
	private:
		ComPtr<ID3D11Device> m_device;
		ComPtr<ID3D11DeviceContext> m_context;
		
		std::shared_ptr<CameraActor> m_mainCamera;
		std::shared_ptr<TriangleActor> triangle;

		// Shader에서 공통으로 사용되는 Constant Buffer Data
		GlobalConstants m_globalConstsCPU;
		ComPtr<ID3D11Buffer> m_globalConstsGPU;

		InputAxis axis = InputAxis::ZAxis;
		InputButton w = InputButton::W, s = InputButton::S;
		InputAxisAction action;
	};
}
