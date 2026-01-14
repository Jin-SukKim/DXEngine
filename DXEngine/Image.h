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
		bool LoadExr(const std::string& filename, DXGI_FORMAT& pixelFormat);
		// HDRI 이미지 읽어오기 (HDRI는 보통 IBL할때 주로 사용)

		const std::vector<uint8_t>& GetImage() { return m_image; }
		const int& GetWidth() { return m_width; }
		const int& GetHeight() { return m_height; }
		const int& GetChannels() { return m_channels; }
		const size_t GetSize() { return m_image.size(); }
		
		bool ResizeImage(int targetWidth, int targetHeight);
	private:
		void readImageExr(const std::string& filename, std::vector<uint8_t>& image, int& width, int& height, DXGI_FORMAT& pixelFormat);
	private:
		int m_width = 0;
		int m_height = 0;
		int m_channels = 0;
		std::vector<uint8_t> m_image; // Element하나 당 color의 r, g, b, a로 총 4개의 Index가 1개의 Color값을 가짐 (4 채널인 경우)
	};
}