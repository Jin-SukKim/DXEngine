#include "pch.h"
#include "Image.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

#include <filesystem>
namespace fs = std::filesystem;

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
}