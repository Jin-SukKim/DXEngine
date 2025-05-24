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
		std::vector<uint8_t> m_image; // Element하나 당 color의 r, g, b, a로 총 4개의 Index가 1개의 Color값을 가짐 (4 채널인 경우)
	};
}