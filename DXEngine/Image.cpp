#include "pch.h"
#include "Image.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#include "stb_image_resize2.h"

#include <filesystem>
namespace fs = std::filesystem;
#include <DirectXTexEXR.h> // EXR  HDRI б
#include <fp16.h>

namespace DE {
	bool Image::Load(const std::string& filename)
	{
		fs::path filePath = filename;
		if (!fs::exists(filePath)) //    Ȯ
			return false;

		// Ϸκ ̹ о
		unsigned char* img = stbi_load(filename.c_str(), &m_width, &m_height, &m_channels, 0);
		//std::cout << "ReadImage() " << filename << " " << m_width << " " << m_height << " " << m_channels << std::endl;
			// ε    Ȯ
		if (!img) {
			std::cerr << "Failed to load image: " << filename << std::endl;
			std::cerr << "STB Error: " << stbi_failure_reason() << std::endl;
			return false;
		}

		std::cout << "Image loaded: " << filename << " (" << m_width << "x" << m_height
			<< ", " << m_channels << " channels)" << std::endl;

		//  4äη  
		m_image.resize(m_width * m_height * 4);

		if (m_channels == 0) {
			std::cout << "Cannot read " << m_channels << " channels" << std::endl;
			return false;
		}

		/*if (m_channels == 1) {
			for (size_t i = 0; i < m_width * m_height; ++i) {
				uint8_t r = img[i * m_channels + 0];
				for (size_t c = 0; c < 4; ++c)
					m_image[4 * i + c] = r;
			}
		}
		else if (m_channels == 2) {
			for (size_t i = 0; i < m_width * m_height; ++i) {
				for (size_t c = 0; c < 2; ++c)
					m_image[4 * i + c] = img[i * m_channels + c];
				m_image[4 * i + 2] = 255;
				m_image[4 * i + 3] = 255;
			}
		}
		else if (m_channels == 3) {
			for (size_t i = 0; i < m_width * m_height; ++i) {
				for (size_t c = 0; c < 3; ++c)
					m_image[4 * i + c] = img[i * m_channels + c];
				m_image[4 * i + 3] = 255;
			}
		}
		else if (m_channels == 4) {
			for (size_t i = 0; i < m_width * m_height; ++i) {
				for (size_t c = 0; c < 4; ++c)
					m_image[4 * i + c] = img[i * m_channels + c];
			}
		}*/

		for (size_t i = 0; i < m_width * m_height; ++i) {
			for (size_t c = 0; c < m_channels; ++c)
				m_image[4 * i + c] = img[i * m_channels + c];
			for (size_t c = m_channels; c < 4; ++c)
				m_image[4 * i + c] = 255;
		}
		
		stbi_image_free(img);
		m_channels = 4;

		return true;
	}
	bool Image::LoadExr(const std::string& filename, DXGI_FORMAT& pixelFormat)
	{
		readImageExr(filename, m_image, m_width, m_height, pixelFormat);
		if (!m_width || !m_height || m_image.empty())
			return false;
		return true;
	}

	bool Image::GenerateRadialGradientPNG(
		const std::string& outputPath, int size,
		const Vector3& centerColor, const Vector3& edgeColor, float falloff)
	{
		std::vector<uint8_t> pixels(size * size * 4);
		float center = size / 2.0f;

		for (int y = 0; y < size; ++y) {
			for (int x = 0; x < size; ++x) {
				float dx = (x + 0.5f - center) / center;
				float dy = (y + 0.5f - center) / center;
				float dist = sqrtf(dx * dx + dy * dy);
				float t = std::clamp(1.0f - powf(std::clamp(dist, 0.f, 1.f), falloff), 0.f, 1.f);

				Vector3 color = Vector3::Lerp(edgeColor, centerColor, t);
				int idx = (y * size + x) * 4;
				pixels[idx + 0] = (uint8_t)(std::clamp(color.x, 0.f, 1.f) * 255);
				pixels[idx + 1] = (uint8_t)(std::clamp(color.y, 0.f, 1.f) * 255);
				pixels[idx + 2] = (uint8_t)(std::clamp(color.z, 0.f, 1.f) * 255);
				pixels[idx + 3] = (uint8_t)(t * 255);
			}
		}

		return stbi_write_png(outputPath.c_str(), size, size, 4, pixels.data(), size * 4) != 0;
	}

	void Image::readImageExr(const std::string& filename, std::vector<uint8_t>& image, int& width, int& height, DXGI_FORMAT& pixelFormat)
	{
		const std::wstring wFilename(filename.begin(), filename.end());

		DirectX::TexMetadata metadata;
		ThrowIfFailed(DirectX::GetMetadataFromEXRFile(wFilename.c_str(), metadata));

		DirectX::ScratchImage scratchImage;
		//  ̹ ͸ о (exr  float Format  r, g, b, a  16-bit float )
		ThrowIfFailed(DirectX::LoadFromEXRFile(wFilename.c_str(), NULL, scratchImage));

		width = static_cast<int>(metadata.width);
		height = static_cast<int>(metadata.height);
		pixelFormat = metadata.format; // DirectX ȣȯǴ Library̱   FOrmat 

		std::cout << filename << " " << metadata.width << " " << metadata.height << " " << metadata.format << std::endl;

		// image uint8_t 8bit¥ uint  迭 ϴ о ޸𸮸 
		image.resize(scratchImage.GetPixelsSize());
		memcpy(image.data(), scratchImage.GetPixels(), image.size());

		//   Ȯ
		// exr ʹ rgba  16-bit float  C++ float 32bit ũ⸦ 
		// float ǥ 16bit ؼ  ߵǱ⿡ GPU 16bit¥  ȭ ߵǾ 
		// , C++ half-float(16bit) ⿡ ܺ library  (debugging 16bits float 32bits ȯ)
		std::vector<float> f32(image.size() / 2);
		uint16_t* f16 = (uint16_t*)image.data();
		for (int i = 0; i < image.size() / 2; ++i)
			f32[i] = fp16_ieee_to_fp32_value(f16[i]);

		const float minValue = *std::min_element(f32.begin(), f32.end());
		const float maxValue = *std::max_element(f32.begin(), f32.end());

		std::cout << minValue << " " << maxValue << std::endl;

		// f16 = (uint16_t *)image.data();
		// for (int i = 0; i < image.size() / 2; i++) {
		//     f16[i] = fp16_ieee_from_fp32_value(f32[i] * 2.0f);
		// }
	}
}