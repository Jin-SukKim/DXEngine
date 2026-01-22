#include "pch.h"
#include "Image2.h"

namespace DE {
	bool Image2::Load(const std::string& filename)
	{
		std::filesystem::path path(filename);
		std::string ext = path.extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });

		HRESULT hr = S_OK;
		if (ext == ".dds") 
			hr = DirectX::LoadFromDDSFile(path.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, m_scratchImage);
		else if (ext == ".tga") 
			hr = DirectX::LoadFromTGAFile(path.c_str(), nullptr, m_scratchImage);
		else if (ext == ".exr") 
			hr = DirectX::LoadFromTGAFile(path.c_str(), nullptr, m_scratchImage);
		else
			hr = DirectX::LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, m_scratchImage);

		if (FAILED(hr)) {
			std::cout << "[Image2] Failed to load: " << filename << std::endl;
			return false;
		}

		// [핵심] DDS 압축 해제 로직 추가
		// 텍스처가 압축된 상태라면 Resize가 불가능하므로 압축을 풉니다.
		if (DirectX::IsCompressed(m_scratchImage.GetMetadata().format))
		{
			DirectX::ScratchImage decompressed;
			hr = DirectX::Decompress(
				m_scratchImage.GetImages(),
				m_scratchImage.GetImageCount(),
				m_scratchImage.GetMetadata(),
				DXGI_FORMAT_UNKNOWN, // 포맷 자동 선택 (보통 R8G8B8A8_UNORM)
				decompressed
			);

			if (SUCCEEDED(hr)) {
				m_scratchImage = std::move(decompressed);
			}
			else {
				std::cout << "[Image2] Failed to decompress DDS: " << filename << std::endl;
				return false;
			}
		}

		return true;
	}

	bool Image2::Resize(size_t targetWidth, size_t targetHeight)
	{
		if (m_scratchImage.GetImageCount() == 0) return false;

		const auto& meta = m_scratchImage.GetMetadata();
		if (meta.width == targetWidth && meta.height == targetHeight) return true;

		// [핵심] 밉맵이 있더라도 첫 번째 이미지(Mip 0)만 리사이징하도록 메타데이터 설정
		DirectX::TexMetadata singleMeta = meta;
		singleMeta.mipLevels = 1;
		singleMeta.arraySize = 1;

		DirectX::ScratchImage resizedImage;
		HRESULT hr = DirectX::Resize(
			m_scratchImage.GetImages(), // 이미지 배열의 첫 번째 포인터(Mip0)를 사용
			1,                          // 1장만 처리 (nimages = 1)
			singleMeta,                 // 메타데이터도 1장짜리로 전달
			targetWidth,
			targetHeight,
			DirectX::TEX_FILTER_DEFAULT,
			resizedImage
		);

		if (FAILED(hr)) {
			// 에러 코드 확인용 (디버깅 시 유용)
			std::cout << "[Image2] Resize failed. HRESULT: " << std::hex << hr << std::endl;
			return false;
		}

		m_scratchImage = std::move(resizedImage);
		return true;
	}

	bool Image2::Convert(DXGI_FORMAT targetFormat)
	{
		if (m_scratchImage.GetMetadata().format == targetFormat) return true;

		DirectX::ScratchImage convertedImage;
		HRESULT hr = DirectX::Convert(
			m_scratchImage.GetImages(), 
			m_scratchImage.GetImageCount(),
			m_scratchImage.GetMetadata(),
			targetFormat, 
			DirectX::TEX_FILTER_DEFAULT, 
			DirectX::TEX_THRESHOLD_DEFAULT, 
			convertedImage
		);

		if (FAILED(hr)) return false;

		m_scratchImage = std::move(convertedImage);
		return true;
	}
}