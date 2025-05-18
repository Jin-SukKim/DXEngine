#pragma once
#include "Actor.h"

#include "Mesh.h"
#include "ConstantBuffers.h"

namespace DE {
	class TriangleActor : public Actor
	{
		using Super = Actor;
	public:
		TriangleActor(const std::wstring& name) : Super(name) {}
		virtual ~TriangleActor() override {}

		void Initialize(ComPtr<ID3D11Device>& device) override;
		void Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime) override;
		void Render(ComPtr<ID3D11DeviceContext>& context) override;
	private:
		Mesh triangle;

		MeshConstants constantData;
		ComPtr<ID3D11InputLayout> il;
		ComPtr<ID3D11VertexShader> vs;
		ComPtr<ID3D11PixelShader> ps;
	};
}