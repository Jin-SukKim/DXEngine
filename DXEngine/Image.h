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
		// HDRI ̹ о (HDRI  IBLҶ ַ )

		const std::vector<uint8_t>& GetImage() { return m_image; }
		const int& GetWidth() { return m_width; }
		const int& GetHeight() { return m_height; }
		const int& GetChannels() { return m_channels; }
		const size_t GetSize() { return m_image.size(); }

		// Generate radial gradient PNG (for emissive textures)
		static bool GenerateRadialGradientPNG(
			const std::string& outputPath,
			int size,                           // square resolution (e.g. 128)
			const Vector3& centerColor,         // center color (e.g. {1,1,1} white)
			const Vector3& edgeColor,           // edge color (e.g. {1,0.8,0} yellow)
			float falloff = 1.0f                // falloff curve (1.0=linear, >1=sharp, <1=soft)
		);
	private:
		void readImageExr(const std::string& filename, std::vector<uint8_t>& image, int& width, int& height, DXGI_FORMAT& pixelFormat);
	private:
		int m_width = 0;
		int m_height = 0;
		int m_channels = 0;
		std::vector<uint8_t> m_image; // Elementϳ  color r, g, b, a  4 Index 1 Color  (4 ä )
	};
}