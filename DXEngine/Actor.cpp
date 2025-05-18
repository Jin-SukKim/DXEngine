#include "pch.h"
#include "Actor.h"
#include "TransformComponent.h"

namespace DE {
	Actor::Actor(const std::wstring& name) : Super(name) {
		// 모든 Actor에 TransformComopnent추가
		initTransform();
	}

	void Actor::Initialize(ComPtr<ID3D11Device>& device)
	{
		for (std::unique_ptr<Component>& component : m_components) {
			Component* comp = component.get();
			if (comp)
				comp->Initialize(device);
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

	void Actor::initTransform()
	{
		TransformComponent* tr = AddComponent<TransformComponent>(L"Transform");
	}
}