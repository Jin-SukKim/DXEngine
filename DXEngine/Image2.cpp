#include "pch.h"
#include "Image2.h"

namespace DE {
	bool Image2::Load(const std::string& filename)
	{
		std::wstring wFilename(filename.begin(), filename.end());
		std::string ext(filename.end() - 3, filename.end());
		std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });

		HRESULT hr = S_OK;
		if (ext == "exr") 
			hr = DirectX::LoadFromDDSFile(wFilename.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, m_scratchImage);
		else if (ext == "tga") 
			hr = DirectX::LoadFromTGAFile(wFilename.c_str(), nullptr, m_scratchImage);
		else if (ext == "exr") 
			hr = DirectX::LoadFromTGAFile(wFilename.c_str(), nullptr, m_scratchImage);
		else
			hr = DirectX::LoadFromWICFile(wFilename.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, m_scratchImage);

		if (FAILED(hr)) {
			std::cout << "[Image2] Failed to load: " << filename << std::endl;
			return false;
		}
		return true;
	}

	bool Image2::Resize(size_t targetWidth, size_t targetHeight)
	{
		if (m_scratchImage.GetImageCount() == 0)
			return false;

		const auto& meta = m_scratchImage.GetMetadata();
		if (meta.width == targetWidth && meta.height == targetHeight) return true;

		DirectX::ScratchImage resizedImage;
		HRESULT hr = DirectX::Resize(
			m_scratchImage.GetImages(), 
			m_scratchImage.GetImageCount(), 
			meta, 
			targetWidth, 
			targetHeight, 
			DirectX::TEX_FILTER_DEFAULT, 
			resizedImage);

		if (FAILED(hr)) return false;

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