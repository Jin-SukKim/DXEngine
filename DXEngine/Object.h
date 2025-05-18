#pragma once

namespace DE {
	class Object
	{
	public:
		Object() {}
		Object(const std::wstring& name) : m_name(name) {}
		virtual ~Object() {}

		virtual void Initialize(ComPtr<ID3D11Device>& device) = 0;
		virtual void Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime) = 0;
		virtual void Render(ComPtr<ID3D11DeviceContext>& context) = 0;

		void SetName(const std::wstring& name) { m_name = name; }
		const std::wstring& GetName() { return m_name; }
	private:
		std::wstring m_name = L"";
	};
}