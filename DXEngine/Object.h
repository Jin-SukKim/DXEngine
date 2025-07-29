#pragma once

namespace DE {
	class RenderBase;
	class Object
	{
	public:
		Object() = delete;
		Object(const std::wstring& name) : m_name(name) {}
		virtual ~Object() {}

		virtual void Initialize() = 0;
		virtual void Update(const float& deltaTime) = 0;
		virtual void Render() {}

		void SetName(const std::wstring& name) { m_name = name; }
		const std::wstring& GetName() { return m_name; }
	private:
		std::wstring m_name = L"";
	};
}