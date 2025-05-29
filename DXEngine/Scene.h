#pragma once
//
//#include "InputTypes.h"
//#include "InputAction.h"
#include "InputManager.h"

namespace DE {
	class SampleActor;
	class CameraActor;
	class SkyboxActor;

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
		void UpdateLight(const float& deltaTime);

		void MoveForward(float axis);
	private:
		ComPtr<ID3D11Device> m_device;
		ComPtr<ID3D11DeviceContext> m_context;
		
		std::shared_ptr<CameraActor> m_mainCamera;

		// Shader에서 공통으로 사용되는 Constant Buffer Data
		GlobalConstants m_globalConstsCPU;
		ComPtr<ID3D11Buffer> m_globalConstsGPU;

		InputAxis axis = InputAxis::ZAxis;
		InputButton w = InputButton::W, s = InputButton::S;
		InputAxisAction action;

		ComPtr<ID3D11InputLayout> il;
		ComPtr<ID3D11VertexShader> vs;
		ComPtr<ID3D11PixelShader> ps;

		ComPtr<ID3D11SamplerState> m_linearWrap;

		// Normal Vector
		ComPtr<ID3D11VertexShader> normalVS;
		ComPtr<ID3D11GeometryShader> normalGS;
		ComPtr<ID3D11PixelShader> normalPS;

		std::shared_ptr<SampleActor> triangle;

		// IBL
		ComPtr<ID3D11VertexShader> m_skyboxVS;
		ComPtr<ID3D11PixelShader> m_skyboxPS;

		std::shared_ptr<SkyboxActor> m_skybox;

	};
}
