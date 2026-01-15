#include "pch.h"
#include "D3D11Utils.h"
#include "Image.h"
#include "Texture2D.h"
#include "Image2.h"

#include <directxtk/DDSTextureLoader.h>

namespace DE {
	void D3D11Utils::CreateIndexBuffer(ComPtr<ID3D11Device>& device, const std::vector<uint32_t>& indices, ComPtr<ID3D11Buffer>& indexBuffer)
	{
		D3D11_BUFFER_DESC desc = {};
		// 초기화 후 변경 x (Indices 순서는 바뀔일이 없음)
		desc.Usage = D3D11_USAGE_IMMUTABLE; // GPU Read
		desc.ByteWidth = UINT(sizeof(uint32_t) * indices.size());
		desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		desc.CPUAccessFlags = 0; // No CPU Access
		desc.StructureByteStride = sizeof(uint32_t);

		D3D11_SUBRESOURCE_DATA indexData = { 0 };
		indexData.pSysMem = indices.data();
		indexData.SysMemPitch = 0;
		indexData.SysMemSlicePitch = 0;

		device->CreateBuffer(&desc, &indexData, indexBuffer.GetAddressOf());
	}
	void D3D11Utils::CreateVSAndIL(ComPtr<ID3D11Device>& device, const std::wstring& filename, const std::vector<D3D11_INPUT_ELEMENT_DESC>& inputElements, ComPtr<ID3D11VertexShader>& vertexShader, ComPtr<ID3D11InputLayout>& inputLayout)
	{
		// 임시로 사용할 데이터를 저장할 Blob 공간
		ComPtr<ID3DBlob> shaderBlob;

		UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
		compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
		ComPtr<ID3DBlob> errorBlob;

		// 콘솔에 출력
		//std::cout << static_cast<const char*>(errorBlob->GetBufferPointer());
		// D3D_COMPILE_STANDARD_FILE_INCLUDE로 Shader에서 include 사용
		ThrowIfFailed(D3DCompileFromFile(filename.c_str(), 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0", compileFlags, 0, &shaderBlob, &errorBlob));

		device->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, &vertexShader);
		device->CreateInputLayout(inputElements.data(), UINT(inputElements.size()), shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), &inputLayout);
	}
	void D3D11Utils::CreatePS(ComPtr<ID3D11Device>& device, const std::wstring& filename, ComPtr<ID3D11PixelShader>& pixelShader)
	{
		// 임시로 사용할 데이터를 저장할 Blob 공간
		ComPtr<ID3DBlob> shaderBlob;
		ComPtr<ID3DBlob> errorBlob;
		
		UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
		compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif


		ThrowIfFailed(D3DCompileFromFile(filename.c_str(), 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", compileFlags, 0, &shaderBlob, &errorBlob));

		device->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, &pixelShader);
	}

	void D3D11Utils::CreateGS(ComPtr<ID3D11Device>& device, const std::wstring& filename, ComPtr<ID3D11GeometryShader>& geometryShader)
	{
		ComPtr<ID3DBlob> shaderBlob;
		ComPtr<ID3DBlob> errorBlob;

		UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
		compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
		ThrowIfFailed(D3DCompileFromFile(filename.c_str(), 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "gs_5_0", compileFlags, 0, &shaderBlob, &errorBlob));

		device->CreateGeometryShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, &geometryShader);
	}

	void D3D11Utils::CreateHS(ComPtr<ID3D11Device>& device, const std::wstring& filename, ComPtr<ID3D11HullShader>& hullShader)
	{
		ComPtr<ID3DBlob> shaderBlob;
		ComPtr<ID3DBlob> errorBlob;

		UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
		compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
		ThrowIfFailed(D3DCompileFromFile(filename.c_str(), 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "hs_5_0", compileFlags, 0, &shaderBlob, &errorBlob));

		device->CreateHullShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, &hullShader);
	}

	void D3D11Utils::CreateDS(ComPtr<ID3D11Device>& device, const std::wstring& filename, ComPtr<ID3D11DomainShader>& domainShader)
	{
		ComPtr<ID3DBlob> shaderBlob;
		ComPtr<ID3DBlob> errorBlob;

		UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
		compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
		ThrowIfFailed(D3DCompileFromFile(filename.c_str(), 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ds_5_0", compileFlags, 0, &shaderBlob, &errorBlob));

		device->CreateDomainShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, &domainShader);
	}

	void D3D11Utils::CreateTexture(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const std::string& filename, const bool usSRGB, Texture2D& texture)
	{
		Image img(L"Image");

		std::string ext(filename.end() - 3, filename.end());
		std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });

		// HDRI pipeline으로 float을 사용하는데 일반적인 이미지는 UNORM이므로 UNORM을 쓰면 일반적인 Texture가 너무 밝아지는 문제가 발생하기 떄문에 SRGB 포맷을 사용
		// SRGB는 내부적으로 Gamma Correction을 해주기 때문에 HDR하고 같은 공간에서 작업 가능
		DXGI_FORMAT pixelFormat = usSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM; // 일반적인 이미지 파일의 형식은 uint8_t이기에 R8G8B8A8_UNORM 사용
		// image의 확장자가 exr이라면 HDRI란 의미
		if (ext == "exr") {
			// HDRI는 RGBA각 16bit float을 사용하므로 uint16_t * 4의 pixel 크기를 가짐
			if (!img.LoadExr(filename, pixelFormat)) throw std::exception();
		}
		else 
			if (!img.Load(filename)) throw std::exception();
		
		CreateTextureHelper(device, context, img.GetWidth(), img.GetHeight(), img.GetImage(), pixelFormat, texture);
	}

	void D3D11Utils::CreateTexture(ComPtr<ID3D11Device>& device, const ComPtr<ID3D11Texture2D>& resource, Texture2D& texture)
	{
		D3D11_TEXTURE2D_DESC desc;
		resource->GetDesc(&desc);
		desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

		// Resource Texture의 설정을 가져와서 생성
		ThrowIfFailed(device->CreateTexture2D(&desc, NULL, texture.GetAddressOfTexture()));
		// Shader Resource View 생성
		ThrowIfFailed(device->CreateShaderResourceView(texture.GetTexture(), nullptr, texture.GetAddressOfSRV()));
		// Render Target View 생성
		ThrowIfFailed(device->CreateRenderTargetView(texture.GetTexture(), nullptr, texture.GetAddressOfRTV()));
	}

	void D3D11Utils::CreateTexture(ComPtr<ID3D11Device>& device, const D3D11_TEXTURE2D_DESC& desc, Texture2D& texture)
	{
		// Texture2D 생성
		ThrowIfFailed(device->CreateTexture2D(&desc, NULL, texture.GetAddressOfTexture()));
		// Shader Resource View 생성
		ThrowIfFailed(device->CreateShaderResourceView(texture.GetTexture(), nullptr, texture.GetAddressOfSRV()));
		// Render Target View 생성
		ThrowIfFailed(device->CreateRenderTargetView(texture.GetTexture(), nullptr, texture.GetAddressOfRTV()));
	}

	void D3D11Utils::CreateTextureHelper(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const int& width, const int& height, const std::vector<uint8_t>& image, const DXGI_FORMAT& pixelFormat, DE::Texture2D& texture)
	{
		// Staging Texture 만들고 CPU에서 이미지를 복사
		ComPtr<ID3D11Texture2D> stagingTexture;
		CreateStagingTexture(device, context, width, height, stagingTexture, image, pixelFormat);

		// Texture 설정
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = 0; // MipMap Level 최대
		desc.ArraySize = 1;
		desc.Format = pixelFormat;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		// Shader Resource View로 사용
		desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS; // MipMap 사용
		desc.CPUAccessFlags = 0; // No CPU Access

		// 초기 데이터 없이 Texture 생성 (전부 검은색)
		ThrowIfFailed(device->CreateTexture2D(&desc, NULL, texture.GetAddressOfTexture()));

		// 실제로 생성된 MipLevels를 확인해보고 싶을 경우
		// texture->GetDesc(&desc);
		// std::cout << desc.MipLevels << std::endl;

		// Staging Texture로부터 가장 해상도가 높은 이미지 복사
		context->CopySubresourceRegion(texture.GetTexture(), 0, 0, 0, 0, stagingTexture.Get(), 0, nullptr);

		// ResourceView 만들기
		ThrowIfFailed(device->CreateShaderResourceView(texture.GetTexture(), nullptr, texture.GetAddressOfSRV()));

		// 해상도를 낮춰가며 MipMap 생성
		context->GenerateMips(texture.GetSRV());
	}

	void D3D11Utils::CreateImageFilterTexture(ComPtr<ID3D11Device>& device, int width, int height, Texture2D& texture)
	{
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = desc.ArraySize = 1; // Post-Processing에는 Mipmap이 불필요
		desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; // 이미지 처리 용도
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT; // GPU read/write
		// SRV와 RTV 용으로 사용
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		desc.MiscFlags = 0;
		desc.CPUAccessFlags = 0;

		// 데이터 없이 Texture 공간만 설정 및 생성
		ThrowIfFailed(device->CreateTexture2D(&desc, NULL, texture.GetAddressOfTexture()));
		// Shader Resource View 생성
		ThrowIfFailed(device->CreateShaderResourceView(texture.GetTexture(), nullptr, texture.GetAddressOfSRV()));
		// Render Target View 생성
		ThrowIfFailed(device->CreateRenderTargetView(texture.GetTexture(), nullptr, texture.GetAddressOfRTV()));
	}

	void D3D11Utils::CreateDDSTexture(ComPtr<ID3D11Device>& device, const std::wstring& filename, bool isCubeMap, ComPtr<ID3D11ShaderResourceView>& textureResourceView)
	{
		ComPtr<ID3D11Texture2D> texture;

		UINT miscFlags = 0;
		if (isCubeMap)
			miscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE; // Cubemap용 Texture

		// https://github.com/microsoft/DirectXTK/wiki/DDSTextureLoader
		ThrowIfFailed(DirectX::CreateDDSTextureFromFileEx(
			device.Get(), filename.c_str(), 0, D3D11_USAGE_DEFAULT,
			D3D11_BIND_SHADER_RESOURCE, 0, miscFlags, DirectX::DDS_LOADER_FLAGS(false),
			(ID3D11Resource**)texture.GetAddressOf(), 
			textureResourceView.GetAddressOf(), nullptr));
	}

	void D3D11Utils::CreateStagingTexture(ComPtr<ID3D11Device>& device, const int& width, const int& height, ComPtr<ID3D11Texture2D>& texture, const DXGI_FORMAT& pixelFormat)
	{
		// Staginge Texture 생성
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.BindFlags = 0;
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = desc.ArraySize = 1; // GPU의 데이터를 가져올 용도의 Staging Texture이기에 mipmap 불필요
		desc.Format = pixelFormat;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_STAGING; // GPU->CPU로 데이터를 보낼 용도
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE; // CPU에서 접근

		ThrowIfFailed(device->CreateTexture2D(&desc, NULL, texture.GetAddressOf()));
	}

	void D3D11Utils::CreateStagingTexture(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const int& width, const int& height, ComPtr<ID3D11Texture2D>& texture, const std::vector<uint8_t>& image, const DXGI_FORMAT& pixelFormat, const int& mipLevels, const int& arraySize)
	{
		// Staging Texture 생성
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = mipLevels;
		desc.ArraySize = arraySize;
		desc.Format = pixelFormat;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_STAGING;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;

		ThrowIfFailed(device->CreateTexture2D(&desc, NULL, texture.GetAddressOf()));

		// Pixel Format에 맞는 한 픽셀 색의 크기 
		// RGBA 각 8bit씩 사용하면 uint8_t * 4(일반적인 이미지), 각 16bit라면 uint16_t * 4(HDRI) 
		size_t pixelSize = GetPixelSize(pixelFormat);

		// CPU에서 이미지 데이터 복사
		D3D11_MAPPED_SUBRESOURCE ms;
		context->Map(texture.Get(), NULL, D3D11_MAP_WRITE, NULL, &ms);
		uint8_t* pData = (uint8_t*)ms.pData; // uint8_t는 색깔 1개 값 (ex: R값 1개)
		for (UINT h = 0; h < UINT(height); ++h) { 
			// GPU 메모리와 CPU 메모리가 1대1로 대응되지 않기에 가로줄 한 줄씩 복사
			memcpy(&pData[h * ms.RowPitch], &image[h * width * pixelSize], width * pixelSize);
		}
		context->Unmap(texture.Get(), NULL);
	}

	void D3D11Utils::CreateTextureArray(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const std::vector<std::string>& filenames, Texture2D& texture)
	{
		// TODO: 모든 이미지의 width와 height이 같다고 가정
		std::vector<Image> imgs(filenames.size(), Image(L"Textures"));

		// 파일로부터 이미지 여러 개를 읽어들임
		for (size_t i = 0; i < filenames.size(); ++i)
			if (!imgs[i].Load(filenames[i]))
				throw std::exception();

		UINT size = UINT(filenames.size());

		// Texture2DArray를 생성 (이때 데이터를 CPU로부터 복사하지 않음)
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Width = UINT(imgs[0].GetWidth());
		desc.Height = UINT(imgs[0].GetHeight());
		desc.MipLevels = 0; // Mipmap Level 최대
		desc.ArraySize = size; // Texture Array이므로 사용할 Texture 개수
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT; // Staging Texture로부터 복사 가능
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS; // MipMap 사용

		// SUBRESOURCE_DATA의 배열
		//std::vector<D3D11_SUBRESOURCE_DATA> initData(size);
		//size_t offset = 0;
		//size_t pixelSize = GetPixelSize(desc.Format);
		//for (auto& i : initData) {
		//	// 각각의 이미지가 시작하는 시작점
		//	//i.pSysMem = imageArray.data() + offset;
		//	i.pSysMem = img[offset++].GetImage().data();
		//	// 가로줄 하나의 데이터 크기
		//	i.SysMemPitch = desc.Width * pixelSize;
		//	// 2D에선 사용하지 않으나 3D부터는 사용되는 한 면의 크기
		//	i.SysMemSlicePitch = desc.Width * desc.Height * pixelSize; // 이미지 하나의 데이터 크기
		//	//offset += i.SysMemSlicePitch; // 다음 이미지의 시작점을 알기 위한 offset
		//}
		//ThrowIfFailed(device->CreateTexture2D(&desc, initData.data(), texture.GetAddressOf()));

		// 일반적인 Texture2D는 srv desc를 설정 안해도 되나 Texture2D를 Array처럼 사용하기 위해선 설정
		//D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		//ZeroMemory(&srvDesc, sizeof(srvDesc));
		//srvDesc.Format = desc.Format;
		//// Array로 사용하겠다는 설정 핵심
		//srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		//srvDesc.Texture2DArray.MostDetailedMip = 0;
		//srvDesc.Texture2DArray.MipLevels = desc.MipLevels;
		//srvDesc.Texture2DArray.FirstArraySlice = 0;
		//// 얼만큼 큰 Array를 사용할건지 설정해주는 핵심
		//srvDesc.Texture2DArray.ArraySize = desc.ArraySize;
		//ThrowIfFailed(device->CreateShaderResourceView(texture.Get(), &srvDesc, textureSRV.GetAddressOf()));

		// 초기 데이터 없이 Texture 생성
		ThrowIfFailed(device->CreateTexture2D(&desc, nullptr, texture.GetAddressOfTexture()));

		// StagingTexture를 만들어서 하나씩 복사
		for (size_t i = 0; i < imgs.size(); ++i) {
			// StagingTexture는 Texture2DArray가 아니라 Texture2D
			ComPtr<ID3D11Texture2D> stagingTexture;
			CreateStagingTexture(device, context, imgs[i].GetWidth(), imgs[i].GetHeight(), stagingTexture, imgs[i].GetImage());

			// Staging Texture를 Texture 배열의 해당 위치에 복사
			UINT subresourceIndex = D3D11CalcSubresource(0, UINT(i), desc.MipLevels);
			context->CopySubresourceRegion(texture.GetTexture(), subresourceIndex, 0, 0, 0, stagingTexture.Get(), 0, nullptr);
		}

		ThrowIfFailed(device->CreateShaderResourceView(texture.GetTexture(), nullptr, texture.GetAddressOfSRV()));
		
		context->GenerateMips(texture.GetSRV());
	}

	void D3D11Utils::CreateMetallicRoughnessTexture(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const std::string& metallicFilename, const std::string& roughnessFilename, Texture2D& texture)
	{
		// GLTF는 Metallic과 Roughness가 이미 합쳐진 Texture를 사용
		if (!metallicFilename.empty() && (metallicFilename == roughnessFilename))
			CreateTexture(device, context, metallicFilename, false, texture);
		// 다른 Format(fbx 등)의 Metallic, Roughness를 따로 가진 경우는 합쳐서 하나의 Texture로 만들어주기
		else {
			// 별도의 파일일 경우 따로 읽어서 합쳐주기
			
			// Image 클래스를 활용하기 위해서 두 이미지들을 각각 4채널로 변환 후 
			// 다시 3채널로 합치는 방식으로 구현
			Image mImage(L"metallic"); // metallic
			Image rImage(L"roughness"); // Roughness

			int width = 0, height = 0;
			// 만약에 둘 중 하나만 있을 경우도 고려하기 위해서 각각 파일명 확인
			if (!metallicFilename.empty()) {
				if (!mImage.Load(metallicFilename)) throw std::exception();
				width = mImage.GetWidth();
				height = mImage.GetHeight();
			}

			if (!roughnessFilename.empty()) {
				if (!rImage.Load(roughnessFilename)) throw std::exception();
				width = mImage.GetWidth();
				height = mImage.GetHeight();
			}

			// 두 이미지의 해상도가 같다고 가정
			if (!metallicFilename.empty() && !roughnessFilename.empty()) {
				assert(mImage.GetWidth() == rImage.GetWidth());
				assert(mImage.GetHeight() == rImage.GetHeight());
			}

			const std::vector<uint8_t>& metallic = mImage.GetImage();
			const std::vector<uint8_t>& roughness = rImage.GetImage();

			std::vector<uint8_t> combinedImage(mImage.GetSize());
			std::fill(combinedImage.begin(), combinedImage.end(), 0);

			// GLTF에서 G가 Roughness, B가 Metallic으로 사용되기에 통일시켜주기
			size_t pixelSize = size_t(width * height);
			for (size_t i = 0; i < pixelSize; ++i) {
				// Roughness Texture가 있다면
				if (rImage.GetSize())
					combinedImage[4 * i + 1] = roughness[4 * i]; // Green = Roughness
				// metallic Texture가 있다면
				if (mImage.GetSize()) 
					combinedImage[4 * i + 2] = metallic[4 * i]; // Blue = Metalness
			}

			CreateTextureHelper(device, context, width, height, combinedImage, DXGI_FORMAT_R8G8B8A8_UNORM, texture);
		}
	}

	void D3D11Utils::CreateTexture2DArray(ID3D11Device* device, ID3D11DeviceContext* context, const std::vector<std::string>& filenames, UINT targetWidth, UINT targetHeight, const bool useSRGB, ComPtr<ID3D11Texture2D>& outTextureArray, ComPtr<ID3D11ShaderResourceView>& outArraySRV)
	{
		if (filenames.empty())
			return;

		std::vector<Image2> images(filenames.size());

		DXGI_FORMAT pixelFormat = useSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM; // 일반적인 이미지 파일의 형식은 uint8_t이기에 R8G8B8A8_UNORM 사용

		for (size_t i = 0; i < filenames.size(); ++i) {
			if (!images[i].Load(filenames[i]))
				// TODO: Load 실패 시 기본 이미지 사용
				continue;

			images[i].Resize(targetWidth, targetHeight);
			images[i].Convert(pixelFormat);
		}

		// Texture2DArray를 생성 (이때 데이터를 CPU로부터 복사하지 않음)
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Width = targetWidth;
		desc.Height = targetHeight;
		desc.MipLevels = 0; // Mipmap Level 최대
		desc.ArraySize = static_cast<UINT>(filenames.size()); // Texture Array이므로 사용할 Texture 개수
		desc.Format = pixelFormat;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT; // Staging Texture로부터 복사 가능
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS; // MipMap 사용

		ThrowIfFailed(device->CreateTexture2D(&desc, nullptr, outTextureArray.GetAddressOf()));

		UINT realMipLevels = 1 + static_cast<UINT>(std::floor(std::log2(std::max(targetWidth, targetHeight))));

		// StagingTexture를 만들어서 하나씩 복사
		for (size_t i = 0; i < images.size(); ++i) {
			if (images[i].GetBuffer().GetImageCount() == 0) continue;
			// StagingTexture는 Texture2DArray가 아니라 Texture2D
			ComPtr<ID3D11Texture2D> stagingTexture;
			CreateStagingTexture(device, context, &images[i], stagingTexture);

			// Staging Texture를 Texture 배열의 해당 위치에 복사
			UINT subresourceIndex = D3D11CalcSubresource(0, static_cast<UINT>(i), realMipLevels);
			context->CopySubresourceRegion(outTextureArray.Get(), subresourceIndex, 0, 0, 0, stagingTexture.Get(), 0, nullptr);
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = desc.Format;
		// Array로 사용하겠다는 설정 핵심
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.MipLevels = -1;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		// 얼만큼 큰 Array를 사용할건지 설정해주는 핵심
		srvDesc.Texture2DArray.ArraySize = desc.ArraySize;
		ThrowIfFailed(device->CreateShaderResourceView(outTextureArray.Get(), &srvDesc, outArraySRV.GetAddressOf()));

		context->GenerateMips(outArraySRV.Get());
	}

	void D3D11Utils::CreateStagingTexture(ID3D11Device* device, ID3D11DeviceContext* context, const Image2* image, ComPtr<ID3D11Texture2D>& outStagingTexture)
	{
		// Image2 내부의 DirectXTex 데이터 가져오기
		const DirectX::ScratchImage& scratchImg = image->GetBuffer();
		const DirectX::Image* imgData = scratchImg.GetImages(); // 첫 번째 이미지(Mip0, Slice0)

		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = static_cast<UINT>(imgData->width);
		desc.Height = static_cast<UINT>(imgData->height);
		desc.MipLevels = 1; // Staging은 복사 용도이므로 MipMap 불필요 (원본 1장만)
		desc.ArraySize = 1;
		desc.Format = imgData->format;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_STAGING;
		desc.BindFlags = 0; // Staging은 Bind Flag 없음
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;

		ThrowIfFailed(device->CreateTexture2D(&desc, nullptr, outStagingTexture.GetAddressOf()));

		// 데이터 복사 (Map/Unmap)
		D3D11_MAPPED_SUBRESOURCE ms;
		ThrowIfFailed(context->Map(outStagingTexture.Get(), 0, D3D11_MAP_WRITE, 0, &ms));

		// 메모리 복사 (Row Pitch를 고려하여 한 줄씩 복사)
		const uint8_t* srcPtr = imgData->pixels;
		uint8_t* destPtr = static_cast<uint8_t*>(ms.pData);

		// 복사할 높이만큼 반복
		for (size_t h = 0; h < imgData->height; ++h)
		{
			// min(Source Pitch, Dest Pitch) 만큼 복사해야 안전함
			// 보통 DirectXTex로 로드하면 포맷이 같아 Pitch도 비슷하지만, Dest가 더 클 수 있음
			size_t copySize = std::min<size_t>(imgData->rowPitch, ms.RowPitch);
			memcpy(destPtr, srcPtr, copySize);

			srcPtr += imgData->rowPitch;
			destPtr += ms.RowPitch;
		}

		context->Unmap(outStagingTexture.Get(), 0);
	}

	void D3D11Utils::CopyFromStagingTexture(ComPtr<ID3D11DeviceContext>& context, const ComPtr<ID3D11Texture2D>& texture, UINT size, void* dest)
	{
		D3D11_MAPPED_SUBRESOURCE ms;
		context->Map(texture.Get(), NULL, D3D11_MAP_READ, NULL, &ms);
		memcpy(dest, ms.pData, size);
		context->Unmap(texture.Get(), NULL);
	}

	size_t D3D11Utils::GetPixelSize(const DXGI_FORMAT& pixelFormat)
	{
		switch (pixelFormat) {
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
			return sizeof(uint16_t) * 4;
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
			return sizeof(uint32_t) * 4;
		case DXGI_FORMAT_R32_FLOAT:
			return sizeof(uint32_t) * 1;
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			return sizeof(uint8_t) * 4;
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			return sizeof(uint8_t) * 4;
		case DXGI_FORMAT_R32_SINT:
			return sizeof(int32_t) * 1;
		case DXGI_FORMAT_R16_FLOAT:
			return sizeof(uint16_t) * 1;
		}

		std::cout << "PixelFormat not implemented " << pixelFormat << std::endl;

		return sizeof(uint8_t) * 4;
	}
	void D3D11Utils::CreateStructuredBuffer(ID3D11Device* device, const UINT numElements, const UINT elementSize, const void* initData, ComPtr<ID3D11Buffer>& buffer, ComPtr<ID3D11ShaderResourceView>& srv, ComPtr<ID3D11UnorderedAccessView>& uav)
	{
		// Structured Buffer 생성
		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.ByteWidth = numElements * elementSize;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | 
						D3D11_BIND_UNORDERED_ACCESS;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		bufferDesc.StructureByteStride = elementSize;

		if (initData) {
			D3D11_SUBRESOURCE_DATA data = {};
			data.pSysMem = initData;
			data.SysMemPitch = 0;
			data.SysMemSlicePitch = 0;

			ThrowIfFailed(device->CreateBuffer(&bufferDesc, &data, buffer.GetAddressOf()));
		}
		else 
			ThrowIfFailed(device->CreateBuffer(&bufferDesc, NULL, buffer.GetAddressOf()));
		
		// SRV 생성
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = numElements;
		ThrowIfFailed(device->CreateShaderResourceView(buffer.Get(), &srvDesc, srv.GetAddressOf()));

		// UAV 생성
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = numElements;
		uavDesc.Buffer.Flags = 0;
		ThrowIfFailed(device->CreateUnorderedAccessView(buffer.Get(), &uavDesc, uav.GetAddressOf()));
	}

	void D3D11Utils::CreateStagingBuffer(ID3D11Device* device, const UINT numElements, const UINT elementSize, const void* initData, ComPtr<ID3D11Buffer>& buffer)
	{
		// StagingBuffer 생성
		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.ByteWidth = numElements * elementSize;
		bufferDesc.Usage = D3D11_USAGE_STAGING;
		bufferDesc.BindFlags = 0;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = elementSize;

		if (initData) {
			D3D11_SUBRESOURCE_DATA data = {};
			data.pSysMem = initData;
			data.SysMemPitch = 0;
			data.SysMemSlicePitch = 0;

			ThrowIfFailed(device->CreateBuffer(&bufferDesc, &data, buffer.GetAddressOf()));
		}
		else
			ThrowIfFailed(device->CreateBuffer(&bufferDesc, NULL, buffer.GetAddressOf()));

	}
	void D3D11Utils::CopyToStagingBuffer(ID3D11DeviceContext* context, ID3D11Buffer* dest, UINT size, void* src)
	{
		D3D11_MAPPED_SUBRESOURCE ms = {};
		context->Map(dest, NULL, D3D11_MAP_WRITE, NULL, &ms);
		memcpy(ms.pData, src, size);
		context->Unmap(dest, NULL);
	}
	void D3D11Utils::CopyFromStagingBuffer(ID3D11DeviceContext* context, void* dest, UINT size, ID3D11Buffer* src)
	{
		D3D11_MAPPED_SUBRESOURCE ms = {};
		context->Map(src, NULL, D3D11_MAP_READ, NULL, &ms);
		memcpy(dest, ms.pData, size);
		context->Unmap(src, NULL);
	}
	void D3D11Utils::CreateCS(ID3D11Device* device, const std::wstring& filename, ComPtr<ID3D11ComputeShader>& computeShader)
	{
		ComPtr<ID3DBlob> shaderBlob;
		ComPtr<ID3DBlob> errorBlob;

		UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
		compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
		ThrowIfFailed(D3DCompileFromFile(
			filename.c_str(), 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, 
			"main", "cs_5_0", compileFlags, 0, &shaderBlob, &errorBlob));

		ThrowIfFailed(device->CreateComputeShader(
			shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(),
			NULL, &computeShader));
	}
	void D3D11Utils::CreateAppendBuffer(ID3D11Device* device, const UINT numElements, const UINT elementSize, const void* initData, ComPtr<ID3D11Buffer>& buffer, ComPtr<ID3D11ShaderResourceView>& srv, ComPtr<ID3D11UnorderedAccessView>& uav)
	{
		// Structured Buffer 생성
		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.ByteWidth = numElements * elementSize;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE |
			D3D11_BIND_UNORDERED_ACCESS;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		bufferDesc.StructureByteStride = elementSize;

		if (initData) {
			D3D11_SUBRESOURCE_DATA data = {};
			data.pSysMem = initData;
			data.SysMemPitch = 0;
			data.SysMemSlicePitch = 0;

			ThrowIfFailed(device->CreateBuffer(&bufferDesc, &data, buffer.GetAddressOf()));
		}
		else
			ThrowIfFailed(device->CreateBuffer(&bufferDesc, NULL, buffer.GetAddressOf()));

		// SRV 생성
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = numElements;
		ThrowIfFailed(device->CreateShaderResourceView(buffer.Get(), &srvDesc, srv.GetAddressOf()));

		// UAV 생성
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = numElements;
		uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_APPEND;
		ThrowIfFailed(device->CreateUnorderedAccessView(buffer.Get(), &uavDesc, uav.GetAddressOf()));
	}

	void D3D11Utils::CreateIndirectBuffer(ID3D11Device* device, UINT byteWidth, const void* initData, ComPtr<ID3D11Buffer>& buffer, ComPtr<ID3D11UnorderedAccessView>& uav)
	{
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = byteWidth;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
		desc.StructureByteStride = 0; 

		if (initData) {
			D3D11_SUBRESOURCE_DATA data = {};
			data.pSysMem = initData;
			ThrowIfFailed(device->CreateBuffer(&desc, &data, buffer.GetAddressOf()));
		}
		else {
			ThrowIfFailed(device->CreateBuffer(&desc, NULL, buffer.GetAddressOf()));
		}

		// UAV 생성 (R32_UINT 포맷 사용)
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_R32_UINT; // uint로 읽기 위해 R32_UINT 사용
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = byteWidth / 4; // UINT 개수 (Byte / 4)
		uavDesc.Buffer.Flags = 0;

		ThrowIfFailed(device->CreateUnorderedAccessView(buffer.Get(), &uavDesc, uav.GetAddressOf()));
	}

	void D3D11Utils::CreateBuffer(ID3D11Device* device, const UINT elementSize, const void* initData, DXGI_FORMAT format, ComPtr<ID3D11Buffer>& buffer, ComPtr<ID3D11ShaderResourceView>& srv)
	{
		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.ByteWidth = elementSize;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.BindFlags =  D3D11_BIND_SHADER_RESOURCE;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;

		if (initData) {
			D3D11_SUBRESOURCE_DATA data = {};
			data.pSysMem = initData;
			data.SysMemPitch = 0;
			data.SysMemSlicePitch = 0;

			ThrowIfFailed(device->CreateBuffer(&bufferDesc, &data, buffer.GetAddressOf()));
		}
		else
			ThrowIfFailed(device->CreateBuffer(&bufferDesc, NULL, buffer.GetAddressOf()));

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.NumElements = 1;
		device->CreateShaderResourceView(buffer.Get(), &srvDesc, srv.GetAddressOf());

		/*D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = format; 
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = 1;
		uavDesc.Buffer.Flags = 0;

		device->CreateUnorderedAccessView(buffer.Get(), &uavDesc, uav.GetAddressOf());*/
	}
}