#include "pch.h"
#include "Actor.h"
#include "TransformComponent.h"
#include "RenderBase.h"
#include "ModelComponent.h"

namespace DE {
	UINT Actor::nextID = 0;

	Actor::Actor(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const std::wstring& name) : Super(device, name), m_id(nextID++) {
		m_components.resize(static_cast<size_t>(ComponentType::MaxComponentType));
		// 모든 Actor에 TransformComopnent추가
		initTransform(device);
		
		setHashIdToColor(GetHashID());
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

	void Actor::Render(RenderBase& renderer)
	{
		ComPtr<ID3D11DeviceContext>& context = renderer.GetContext();

		RenderComponent(context, ComponentType::Model);
		

		//for (std::unique_ptr<Component>& component : m_components) {
		//	Component* comp = component.get();
		//	if (comp)
		//		comp->Render(context);
		//}
	}

	void Actor::RenderBoundingVolume(RenderBase& renderer)
	{
		RenderComponent(renderer.GetContext(), ComponentType::BoundingVolume);
	}

	void Actor::RenderNormal(RenderBase& renderer)
	{
		ModelComponent* comp = dynamic_cast<ModelComponent*>(m_components[size_t(ComponentType::Model)].get());
		if (comp && comp->IsDrawNormal()) {
			// Normal Vector 그리기
			comp->RenderNormal(renderer.GetContext());
		}
	}

	void Actor::initTransform(ComPtr<ID3D11Device>& device)
	{
		TransformComponent* tr = AddComponent<TransformComponent>(device, L"Transform");
	}

	int Actor::GetHashID()
	{
		//std::hash<std::wstring> hash;
		//return (int)hash(GetName()) + m_id;
		return m_id;
	}

	void Actor::setHashIdToColor(const int& hashID)
	{
		// 0xff = 255 (8 bit)
		m_hashColor[0] = (hashID >> 16) & 0xff;	// r
		m_hashColor[1] = (hashID >> 8) & 0xff;	// g
		m_hashColor[2] = hashID & 0xff;			// b
		m_hashColor[3] = 255;						// a
	}

	void Actor::RenderComponent(ComPtr<ID3D11DeviceContext>& context, const ComponentType& type)
	{
		Component* comp = m_components[size_t(type)].get();
		if (comp)
			comp->Render(context);
	}
}