#pragma once
#include "Object.h"

namespace DE {
	class Component : public Object
	{
		using Super = Object;
	public:
		Component(const std::wstring& name, ComponentType type) : Super(name), m_type(type) {}
		~Component() override {}

		virtual void Initialize() override {}
		virtual void Update(const float& deltaTime) override {}
		virtual void UpdateGui() override {}
		virtual void Render() override  { if (!m_show) return; }

		void SetOwner(Object* owner) { m_owner = owner; }
		Object* GetOwner() const { return m_owner; }
		const ComponentType& GetType() const { return m_type; }

		void SetVisibility(bool show) { m_show = show; }
	private:
		Object* m_owner = nullptr;
		bool m_show = true;
		ComponentType m_type;
	};
}
