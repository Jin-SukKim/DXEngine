#pragma once
#include "pch.h"
#include "Object.h"

namespace DE {
	class Resource {
	public:
		Resource(const std::wstring& name) : m_name(name) {}
		virtual ~Resource() {}

		virtual bool Load(const std::string& filename) = 0;
		void SetName(const std::wstring& name) { m_name = name; }
		const std::wstring& GetName() { return m_name; }
	private:
		std::wstring m_name = L"";
	};
}