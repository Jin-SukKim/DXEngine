#pragma once
#include "Object.h"

namespace DE {
	class Component : public Object
	{
		using Super = Object;
	public:
		Component(ComPtr<ID3D11Device>& device, const std::wstring& name) : Super(device, name) {}
		~Component() override {}

		virtual void Initialize() override {}
		virtual void Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime) override {}
		virtual void Render(ComPtr<ID3D11DeviceContext>& context) override  { if (m_show) return; }

		void SetOwner(Object* owner) { m_owner = owner; }
		Object* GetOwner() const { return m_owner; }

		void SetVisibility(bool show) { m_show = show; }
	private:
		Object* m_owner = nullptr;
		bool m_show = true;
	};
}
