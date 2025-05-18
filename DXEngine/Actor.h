#pragma once
#include "Object.h"
#include "Component.h"

namespace DE {
	class Actor : public Object
	{
		using Super = Object;
	public:
		Actor(const std::wstring& name);
		virtual ~Actor() override {}

		virtual void Initialize(ComPtr<ID3D11Device>& device) override;
		virtual void Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime) override;
		virtual void Render(ComPtr<ID3D11DeviceContext>& context) override;

		template<typename T_COMPONENT>
		T_COMPONENT* AddComponent(const std::wstring& name);

		template<typename T_COMPONENT>
		T_COMPONENT* GetComponent();

	private:
		std::vector<std::unique_ptr<Component>> m_components;

		// TransformComponent 추가
		void initTransform();
	};

	template<typename T_COMPONENT>
	inline T_COMPONENT* Actor::AddComponent(const std::wstring& name)
	{
		std::unique_ptr<T_COMPONENT> comp = std::make_unique<T_COMPONENT>(name);
		comp->SetOwner(this);
		m_components.emplace_back(std::move(comp));
		return dynamic_cast<T_COMPONENT*>(m_components.back().get());
	}
	template<typename T_COMPONENT>
	inline T_COMPONENT* Actor::GetComponent()
	{
		T_COMPONENT* comp = nullptr;
		for (std::unique_ptr<Component>& component : m_components) {
			comp = dynamic_cast<T_COMPONENT*>(component.get());
			// 원하는 Component를 찾았다면
			if (comp)
				break;
		}
		return comp;
	}
}
