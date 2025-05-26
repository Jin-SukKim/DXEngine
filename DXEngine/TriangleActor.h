#pragma once
#include "Actor.h"

#include "Mesh.h"

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
		//Mesh triangle;
		std::vector<Mesh> m_meshes; // 하나의 모델이 내부적으로는 여러개의 메쉬로 구성

		MeshConstants m_constantCPU;
		BasicMaterialConstants m_basicMaterialCPU;

		ComPtr<ID3D11InputLayout> il;
		ComPtr<ID3D11VertexShader> vs;
		ComPtr<ID3D11PixelShader> ps;

		ComPtr<ID3D11SamplerState> m_samplerState;

		// Normal Vector
		ComPtr<ID3D11VertexShader> normalVS;
		ComPtr<ID3D11GeometryShader> normalGS;
		ComPtr<ID3D11PixelShader> normalPS;
		bool m_drawNormal = true;
	};
}