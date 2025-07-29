#pragma once
#include "Object.h"
#include "Component.h"

namespace DE {
	class RenderBase;
	class Actor : public Object
	{
		using Super = Object;
	public:
		Actor(const std::wstring& name);
		virtual ~Actor() override {}

		virtual void Initialize() override;
		virtual void Update(const float& deltaTime) override;
		virtual void Render() override;
		void RenderBoundingVolume();
		void RenderNormal();

		template<typename T_COMPONENT>
		T_COMPONENT* AddComponent(const std::wstring& name);

		template<typename T_COMPONENT>
		T_COMPONENT* GetComponent();

		int GetHashID();
		const uint8_t* GetHashColor() const { return m_hashColor; }

		bool IsVisible() { return m_isVisible; }
		bool IsCastShadow() { return m_castShadow; }
	protected:
		void RenderComponent(const ComponentType& type);
	private:
		static UINT nextID;

		const UINT m_id;
		uint8_t m_hashColor[4];
		std::vector<std::unique_ptr<Component>> m_components;

		// TransformComponent 추가
		void initTransform();
		void setHashIdToColor(const int& hashID);

		bool m_isVisible = true;
		bool m_castShadow = true;
	};

	template<typename T_COMPONENT>
	inline T_COMPONENT* Actor::AddComponent(const std::wstring& name)
	{
		std::unique_ptr<T_COMPONENT> comp = std::make_unique<T_COMPONENT>(name);
		comp->SetOwner(this);
		size_t idx = static_cast<size_t>(comp->GetType());
		m_components[idx] = std::move(comp);
		return dynamic_cast<T_COMPONENT*>(m_components[idx].get());
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
