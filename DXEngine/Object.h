#pragma once

namespace DE {
	class RenderBase;
	class Object
	{
	public:
		Object() = delete;
		Object(ComPtr<ID3D11Device>& device, const std::wstring& name) : m_name(name) {}
		virtual ~Object() {}

		virtual void Initialize() = 0;
		virtual void Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime) = 0;
		virtual void Render(RenderBase& renderer) {}
		virtual void Render(ComPtr<ID3D11DeviceContext>& context) {};

		void SetName(const std::wstring& name) { m_name = name; }
		const std::wstring& GetName() { return m_name; }
	private:
		std::wstring m_name = L"";
	};
}