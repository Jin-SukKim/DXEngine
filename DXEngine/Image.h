#pragma once
#include "Resource.h"

namespace DE {
	class Image : public Resource
	{
		using Super = Resource;
	public:
		Image(const std::wstring& name) : Super(name) {}
		~Image() override {}

		bool Load(const std::string& filename) override;
		static void ReadImage(const std::string& filename, std::vector<uint8_t>& image, int& width, int& height);

		const std::vector<uint8_t>& GetImage() { return m_image; }
		const int& GetWidth() { return m_width; }
		const int& GetHeight() { return m_height; }
		const int& GetChannels() { return m_channels; }
	private:
		int m_width = 0;
		int m_height = 0;
		int m_channels = 0;
		std::vector<uint8_t> m_image; // Element�ϳ� �� color�� r, g, b, a�� �� 4���� Index�� 1���� Color���� ���� (4 ä���� ���)
	};
}