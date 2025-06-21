#pragma once
#include "Actor.h"

namespace DE {
	class ModelComponent;
	class BoundComponent;

	class SampleActor : public Actor
	{
		using Super = Actor;
	public:
		SampleActor(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const std::wstring& name);
		virtual ~SampleActor() override {}

		void Initialize() override;
		void Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime) override;
		void Render(RenderBase& renderer) override;
		void RenderNormal(ComPtr<ID3D11DeviceContext>& context);
		
		bool IsDrawNormal();
	private:
		ModelComponent* m_sample;
		BoundComponent* m_boundVolume;
	};
}