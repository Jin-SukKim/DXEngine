#include "pch.h"
#include "Actor.h"
#include "TransformComponent.h"

namespace DE {
	Actor::Actor(const std::wstring& name) : Super(name) {
		// 모든 Actor에 TransformComopnent추가
		initTransform();
	}

	void Actor::Initialize()
	{
		for (std::unique_ptr<Component>& component : m_components) {
			Component* comp = component.get();
			if (comp)
				comp->Initialize();
		}
	}

	void Actor::Update(const float& deltaTime)
	{
		for (std::unique_ptr<Component>& component : m_components) {
			Component* comp = component.get();
			if (comp)
				comp->Update(deltaTime);
		}
	}

	void Actor::Render()
	{
		for (std::unique_ptr<Component>& component : m_components) {
			Component* comp = component.get();
			if (comp)
				comp->Render();
		}
	}

	void Actor::initTransform()
	{
		TransformComponent* tr = AddComponent<TransformComponent>(L"Transform");
	}
}