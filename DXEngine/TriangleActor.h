#pragma once
#include "Actor.h"

namespace DE {
	class ModelComponent;

	class TriangleActor : public Actor
	{
		using Super = Actor;
	public:
		TriangleActor(ComPtr<ID3D11Device>& device, const std::wstring& name);
		virtual ~TriangleActor() override {}

		void Initialize() override;
		void Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime) override;
		void Render(ComPtr<ID3D11DeviceContext>& context) override;
		void RenderNormal(ComPtr<ID3D11DeviceContext>& context);
		
		bool IsDrawNormal();
	private:
		ModelComponent* m_gelda;
	};
}