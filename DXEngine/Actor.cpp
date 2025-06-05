#include "pch.h"
#include "Actor.h"
#include "TransformComponent.h"
#include "RenderBase.h"

namespace DE {
	Actor::Actor(ComPtr<ID3D11Device>& device, const std::wstring& name) : Super(device, name) {
		m_components.resize(static_cast<size_t>(ComponentType::MaxComponentType));
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

	void Actor::Render(RenderBase& renderer)
	{
		ComPtr<ID3D11DeviceContext>& context = renderer.GetContext();

		renderer.SetPipelineState(RenderBase::graphicsCommon.basic.solidPSO);
		RenderComponent(context, ComponentType::Model);
		
		renderer.SetPipelineState(RenderBase::graphicsCommon.basic.boundPSO);
		RenderComponent(context, ComponentType::BoundingVolume);

		//for (std::unique_ptr<Component>& component : m_components) {
		//	Component* comp = component.get();
		//	if (comp)
		//		comp->Render(context);
		//}
	}

	void Actor::initTransform(ComPtr<ID3D11Device>& device)
	{
		TransformComponent* tr = AddComponent<TransformComponent>(device, L"Transform");
	}
	void Actor::RenderComponent(ComPtr<ID3D11DeviceContext>& context, const ComponentType& type)
	{
		Component* comp = m_components[size_t(type)].get();
		if (comp)
			comp->Render(context);
	}
}