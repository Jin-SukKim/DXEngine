#pragma once

namespace DE {
	class Object
	{
	public:
		Object() {}
		Object(const std::wstring& name) : m_name(name) {}
		virtual ~Object() {}

		virtual void Initialize() = 0;
		virtual void Update(const float& DeltaTime) = 0;
		virtual void Render() = 0;

		void SetName(const std::wstring& name) { m_name = name; }
		const std::wstring& GetName() { return m_name; }
	private:
		std::wstring m_name = L"";
	};
}