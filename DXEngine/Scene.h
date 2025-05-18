#pragma once

namespace DE {
	class TriangleActor;
	class CameraActor;

	class Scene
	{
	public:
		Scene(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context);
		virtual ~Scene() {}
		virtual void Initialize();
		virtual void Update();
		virtual void Render();
		void SetGlobalConsts();

		CameraActor* GetMainCamera() { return m_mainCamera.get(); };
	private:
		ComPtr<ID3D11Device> m_device;
		ComPtr<ID3D11DeviceContext> m_context;
		
		std::shared_ptr<CameraActor> m_mainCamera;
		std::shared_ptr<TriangleActor> triangle;

		GlobalConstants m_globalConstsCPU;
		ComPtr<ID3D11Buffer> m_globalConstsGPU;
	};
}
