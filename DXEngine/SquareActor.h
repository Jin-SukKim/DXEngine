#pragma once
#include "Actor.h"

namespace DE {
	class ModelComponent;

	class SquareActor : public Actor
	{
		using Super = Actor;
	public:
		SquareActor(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const std::wstring& name);
		virtual ~SquareActor() override {}

		void Initialize() override;
		void Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime) override;
		void Render(RenderBase& renderer) override;
	private:
		ModelComponent* m_sample;
	};
}