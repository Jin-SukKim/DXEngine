#pragma once

#include "Mesh.h"
#include "ConstantBuffers.h"

namespace DE {
	class Scene
	{
	public:
		Scene(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context);
		virtual ~Scene() {}
		virtual void Initialize();
		virtual void Update();
		virtual void Render();
	private:
		ComPtr<ID3D11Device> m_device;
		ComPtr<ID3D11DeviceContext> m_context;

		Mesh triangle;

		MeshConstants constantData;
		ComPtr<ID3D11InputLayout> il;
		ComPtr<ID3D11VertexShader> vs;
		ComPtr<ID3D11PixelShader> ps;

	};
}
