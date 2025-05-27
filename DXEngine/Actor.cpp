#include "pch.h"
#include "Actor.h"
#include "TransformComponent.h"

namespace DE {
	Actor::Actor(ComPtr<ID3D11Device>& device, const std::wstring& name) : Super(device, name) {
		// 모든 Actor에 TransformComopnent추가
		initTransform(device);
	}

	void Actor::Initialize()
	{
		for (std::unique_ptr<Component>& component : m_components) {
			Component* comp = component.get();
			if (comp)
				comp->Initialize();
		}
	}

	void Actor::Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime)
	{
		for (std::unique_ptr<Component>& component : m_components) {
			Component* comp = component.get();
			if (comp)
				comp->Update(context, deltaTime);
		}
	}

	void Actor::Render(ComPtr<ID3D11DeviceContext>& context)
	{
		for (std::unique_ptr<Component>& component : m_components) {
			Component* comp = component.get();
			if (comp)
				comp->Render(context);
		}
	}

	void Actor::initTransform(ComPtr<ID3D11Device>& device)
	{
		TransformComponent* tr = AddComponent<TransformComponent>(device, L"Transform");
	}
}