#include "pch.h"
#include "Image.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#include "stb_image_resize2.h"

#include <filesystem>
namespace fs = std::filesystem;
#include <DirectXTexEXR.h> // EXR 형식 HDRI 읽기
#include <fp16.h>

namespace DE {
	bool Image::Load(const std::string& filename)
	{
		fs::path filePath = filename;
		if (!fs::exists(filePath)) // 파일 존재 여부 확인
			return false;

		// 파일로부터 이미지를 읽어오기
		unsigned char* img = stbi_load(filename.c_str(), &m_width, &m_height, &m_channels, 0);
		//std::cout << "ReadImage() " << filename << " " << m_width << " " << m_height << " " << m_channels << std::endl;
			// 로드 실패 시 오류 확인
		if (!img) {
			std::cerr << "Failed to load image: " << filename << std::endl;
			std::cerr << "STB Error: " << stbi_failure_reason() << std::endl;
			return false;
		}

		std::cout << "Image loaded: " << filename << " (" << m_width << "x" << m_height
			<< ", " << m_channels << " channels)" << std::endl;

		// 무조건 4채널로 만들어서 복사
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

	bool Image::ResizeImage(int targetWidth, int targetHeight)
	{
		// 예외 처리: 원본 데이터가 없거나 타겟 크기가 잘못된 경우
		if (m_image.empty() || m_width <= 0 || m_height <= 0) return false;
		if (targetWidth <= 0 || targetHeight <= 0) return false;

		// 리사이징된 데이터를 담을 메모리 할당
		// Load()에서 채널을 무조건 4(RGBA)로 맞췄으므로 크기는 w * h * 4
		std::vector<uint8_t> outputImage(targetWidth * targetHeight * 4);

		// 리사이징 수행 (stb_image_resize2.h 사용)
		// stbir_resize_uint8_srgb: 사람의 눈에 보이는 이미지(Color)를 리사이징할 때 감마 보정을 처리해줌
		// 만약 Normal Map이나 Mask 같은 데이터라면 stbir_resize_uint8_linear 사용 권장
		unsigned char* result = stbir_resize_uint8_srgb(
			m_image.data(), m_width, m_height, 0,             // Input: 버퍼, w, h, stride(0=자동)
			outputImage.data(), targetWidth, targetHeight, 0, // Output: 버퍼, w, h, stride(0=자동)
			STBIR_RGBA                                        // Pixel Layout: RGBA (Load함수에서 4채널 고정했으므로)
		);

		if (!result) {
			std::cerr << "Failed to resize image." << std::endl;
			return false;
		}

		// 기존 데이터 교체 및 메타데이터 업데이트
		// std::move를 사용하여 불필요한 복사 방지
		m_image = std::move(outputImage);
		m_width = targetWidth;
		m_height = targetHeight;
		// m_channels는 Load()에서 4로 고정했으므로 변경 없음

		std::cout << "Image resized to: " << m_width << "x" << m_height << std::endl;

		return true;
	}

	void Image::readImageExr(const std::string& filename, std::vector<uint8_t>& image, int& width, int& height, DXGI_FORMAT& pixelFormat)
	{
		const std::wstring wFilename(filename.begin(), filename.end());

		DirectX::TexMetadata metadata;
		ThrowIfFailed(DirectX::GetMetadataFromEXRFile(wFilename.c_str(), metadata));

		DirectX::ScratchImage scratchImage;
		// 실제 이미지 데이터를 읽어오기 (exr 데이터의 float Format을 사용하고 r, g, b, a 각각 16-bit float을 사용)
		ThrowIfFailed(DirectX::LoadFromEXRFile(wFilename.c_str(), NULL, scratchImage));

		width = static_cast<int>(metadata.width);
		height = static_cast<int>(metadata.height);
		pixelFormat = metadata.format; // DirectX와 호환되는 Library이기 때문레 같은 FOrmat을 사용

		std::cout << filename << " " << metadata.width << " " << metadata.height << " " << metadata.format << std::endl;

		// image는 uint8_t로 8bit짜리 uint 색깔 배열로 일단 읽어들인 메모리를 복사
		image.resize(scratchImage.GetPixelsSize());
		memcpy(image.data(), scratchImage.GetPixels(), image.size());

		// 데이터 범위 확인
		// exr 데이터는 rgba 각각 16-bit float을 사용하지만 C++의 float은 32bit 크기를 사용
		// float으로 표현시 16bit만 사용해서 렌더링이 잘되기에 GPU가 16bit짜리에 대해 최적화가 잘되어 있음
		// 단, C++엔 half-float(16bit)가 없기에 외부 library 사용 (debugging시 16bits float을 32bits로 변환)
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