#pragma once
#include "Object.h"
#include "Component.h"

namespace DE {
	class RenderBase;
	class Actor : public Object
	{
		using Super = Object;
	public:
		Actor(ComPtr<ID3D11Device>& device, const std::wstring& name);
		virtual ~Actor() override {}

		virtual void Initialize() override;
		virtual void Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime) override;
		virtual void Render(RenderBase& renderer) override;

		template<typename T_COMPONENT>
		T_COMPONENT* AddComponent(ComPtr<ID3D11Device>& device, const std::wstring& name);

		template<typename T_COMPONENT>
		T_COMPONENT* GetComponent();

		int GetHashID();
		const uint8_t* GetHashColor() const { return m_hashColor; }

	protected:
		void RenderComponent(ComPtr<ID3D11DeviceContext>& context, const ComponentType& type);
	private:
		static UINT nextID;

		const UINT m_id;
		uint8_t m_hashColor[4];
		std::vector<std::unique_ptr<Component>> m_components;

		// TransformComponent 추가
		void initTransform(ComPtr<ID3D11Device>& device);
		void setHashIdToColor(const int& hashID);
	};

	template<typename T_COMPONENT>
	inline T_COMPONENT* Actor::AddComponent(ComPtr<ID3D11Device>& device, const std::wstring& name)
	{
		std::unique_ptr<T_COMPONENT> comp = std::make_unique<T_COMPONENT>(device, name);
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
