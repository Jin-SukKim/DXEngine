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
		// �ʱ�ȭ �� ���� x (Indices ������ �ٲ����� ����)
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
		// �ӽ÷� ����� �����͸� ������ Blob ����
		ComPtr<ID3DBlob> shaderBlob;

		UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
		compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
		ComPtr<ID3DBlob> errorBlob;

		// �ֿܼ� ���
		//std::cout << static_cast<const char*>(errorBlob->GetBufferPointer());
		// D3D_COMPILE_STANDARD_FILE_INCLUDE�� Shader���� include ���
		ThrowIfFailed(D3DCompileFromFile(filename.c_str(), 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0", compileFlags, 0, &shaderBlob, &errorBlob));

		device->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, &vertexShader);
		device->CreateInputLayout(inputElements.data(), UINT(inputElements.size()), shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), &inputLayout);
	}
	void D3D11Utils::CreatePS(ComPtr<ID3D11Device>& device, const std::wstring& filename, ComPtr<ID3D11PixelShader>& pixelShader)
	{
		// �ӽ÷� ����� �����͸� ������ Blob ����
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

		// HDRI pipeline���� float�� ����ϴµ� �Ϲ����� �̹����� UNORM�̹Ƿ� UNORM�� ���� �Ϲ����� Texture�� �ʹ� ������� ������ �߻��ϱ� ������ SRGB ������ ���
		// SRGB�� ���������� Gamma Correction�� ���ֱ� ������ HDR�ϰ� ���� �������� �۾� ����
		DXGI_FORMAT pixelFormat = usSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM; // �Ϲ����� �̹��� ������ ������ uint8_t�̱⿡ R8G8B8A8_UNORM ���
		// image�� Ȯ���ڰ� exr�̶�� HDRI�� �ǹ�
		if (ext == "exr") {
			// HDRI�� RGBA�� 16bit float�� ����ϹǷ� uint16_t * 4�� pixel ũ�⸦ ����
			if (!img.LoadExr(filename, pixelFormat)) throw std::exception();
		}
		else 
			if (!img.Load(filename)) throw std::exception();
		
		CreateTextureHelper(device, context, img.GetWidth(), img.GetHeight(), img.GetImage(), pixelFormat, texture);
	}

	// 1. [�ٽ� ����] ScratchImage�� �޾� �ؽ�ó�� �����ϴ� �Լ�
	void D3D11Utils::CreateTexture(ID3D11Device* device, ID3D11DeviceContext* context, const DirectX::ScratchImage& image, const DXGI_FORMAT& format, Texture2D& texture)
	{
		const DirectX::Image* imgData = image.GetImages(); // Mip0, Slice0

		// 1. Texture ����
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = static_cast<UINT>(imgData->width);
		desc.Height = static_cast<UINT>(imgData->height);
		desc.MipLevels = 0; // ��ü Mipmap ����
		desc.ArraySize = 1;
		desc.Format = format;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
		desc.CPUAccessFlags = 0;

		// 2. Texture ����
		ThrowIfFailed(device->CreateTexture2D(&desc, nullptr, texture.GetAddressOfTexture()));

		// 3. ������ ���ε� (UpdateSubresource ���)
		context->UpdateSubresource(
			texture.GetTexture(),
			0,
			nullptr,
			imgData->pixels,
			static_cast<UINT>(imgData->rowPitch),
			static_cast<UINT>(imgData->slicePitch)
		);

		// 4. SRV ����
		ThrowIfFailed(device->CreateShaderResourceView(texture.GetTexture(), nullptr, texture.GetAddressOfSRV()));

		// 5. Mipmap ����
		context->GenerateMips(texture.GetSRV());
	}

	// 2. [Wrapper] Image2�� �޾� �� �Լ��� ȣ��
	void D3D11Utils::CreateTexture(ID3D11Device* device, ID3D11DeviceContext* context, const Image2* image, const DXGI_FORMAT& format, Texture2D& texture)
	{
		if (image == nullptr) return;
		// Image2 ������ ScratchImage�� ������ ����
		CreateTexture(device, context, image->GetBuffer(), format, texture);
	}

	void D3D11Utils::CreateTexture(ComPtr<ID3D11Device>& device, const ComPtr<ID3D11Texture2D>& resource, Texture2D& texture)
	{
		D3D11_TEXTURE2D_DESC desc;
		resource->GetDesc(&desc);
		desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

		// Resource Texture�� ������ �����ͼ� ����
		ThrowIfFailed(device->CreateTexture2D(&desc, NULL, texture.GetAddressOfTexture()));
		// Shader Resource View ����
		ThrowIfFailed(device->CreateShaderResourceView(texture.GetTexture(), nullptr, texture.GetAddressOfSRV()));
		// Render Target View ����
		ThrowIfFailed(device->CreateRenderTargetView(texture.GetTexture(), nullptr, texture.GetAddressOfRTV()));
	}

	void D3D11Utils::CreateTexture(ComPtr<ID3D11Device>& device, const D3D11_TEXTURE2D_DESC& desc, Texture2D& texture)
	{
		// Texture2D ����
		ThrowIfFailed(device->CreateTexture2D(&desc, NULL, texture.GetAddressOfTexture()));
		// Shader Resource View ����
		ThrowIfFailed(device->CreateShaderResourceView(texture.GetTexture(), nullptr, texture.GetAddressOfSRV()));
		// Render Target View ����
		ThrowIfFailed(device->CreateRenderTargetView(texture.GetTexture(), nullptr, texture.GetAddressOfRTV()));
	}

	void D3D11Utils::CreateTextureHelper(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const int& width, const int& height, const std::vector<uint8_t>& image, const DXGI_FORMAT& pixelFormat, DE::Texture2D& texture)
	{
		// Staging Texture ����� CPU���� �̹����� ����
		ComPtr<ID3D11Texture2D> stagingTexture;
		CreateStagingTexture(device, context, width, height, stagingTexture, image, pixelFormat);

		// Texture ����
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = 0; // MipMap Level �ִ�
		desc.ArraySize = 1;
		desc.Format = pixelFormat;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		// Shader Resource View�� ���
		desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS; // MipMap ���
		desc.CPUAccessFlags = 0; // No CPU Access

		// �ʱ� ������ ���� Texture ���� (���� ������)
		ThrowIfFailed(device->CreateTexture2D(&desc, NULL, texture.GetAddressOfTexture()));

		// ������ ������ MipLevels�� Ȯ���غ��� ���� ���
		// texture->GetDesc(&desc);
		// std::cout << desc.MipLevels << std::endl;

		// Staging Texture�κ��� ���� �ػ󵵰� ���� �̹��� ����
		context->CopySubresourceRegion(texture.GetTexture(), 0, 0, 0, 0, stagingTexture.Get(), 0, nullptr);

		// ResourceView �����
		ThrowIfFailed(device->CreateShaderResourceView(texture.GetTexture(), nullptr, texture.GetAddressOfSRV()));

		// �ػ󵵸� ���簡�� MipMap ����
		context->GenerateMips(texture.GetSRV());
	}

	void D3D11Utils::CreateImageFilterTexture(ComPtr<ID3D11Device>& device, int width, int height, Texture2D& texture)
	{
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = desc.ArraySize = 1; // Post-Processing���� Mipmap�� ���ʿ�
		desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; // �̹��� ó�� �뵵
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT; // GPU read/write
		// SRV�� RTV ������ ���
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		desc.MiscFlags = 0;
		desc.CPUAccessFlags = 0;

		// ������ ���� Texture ������ ���� �� ����
		ThrowIfFailed(device->CreateTexture2D(&desc, NULL, texture.GetAddressOfTexture()));
		// Shader Resource View ����
		ThrowIfFailed(device->CreateShaderResourceView(texture.GetTexture(), nullptr, texture.GetAddressOfSRV()));
		// Render Target View ����
		ThrowIfFailed(device->CreateRenderTargetView(texture.GetTexture(), nullptr, texture.GetAddressOfRTV()));
	}

	void D3D11Utils::CreateDDSTexture(ComPtr<ID3D11Device>& device, const std::wstring& filename, bool isCubeMap, ComPtr<ID3D11ShaderResourceView>& textureResourceView)
	{
		ComPtr<ID3D11Texture2D> texture;

		UINT miscFlags = 0;
		if (isCubeMap)
			miscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE; // Cubemap�� Texture

		// https://github.com/microsoft/DirectXTK/wiki/DDSTextureLoader
		ThrowIfFailed(DirectX::CreateDDSTextureFromFileEx(
			device.Get(), filename.c_str(), 0, D3D11_USAGE_DEFAULT,
			D3D11_BIND_SHADER_RESOURCE, 0, miscFlags, DirectX::DDS_LOADER_FLAGS(false),
			(ID3D11Resource**)texture.GetAddressOf(), 
			textureResourceView.GetAddressOf(), nullptr));
	}

	void D3D11Utils::CreateStagingTexture(ComPtr<ID3D11Device>& device, const int& width, const int& height, ComPtr<ID3D11Texture2D>& texture, const DXGI_FORMAT& pixelFormat)
	{
		// Staginge Texture ����
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.BindFlags = 0;
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = desc.ArraySize = 1; // GPU�� �����͸� ������ �뵵�� Staging Texture�̱⿡ mipmap ���ʿ�
		desc.Format = pixelFormat;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_STAGING; // GPU->CPU�� �����͸� ���� �뵵
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE; // CPU���� ����

		ThrowIfFailed(device->CreateTexture2D(&desc, NULL, texture.GetAddressOf()));
	}

	void D3D11Utils::CreateStagingTexture(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const int& width, const int& height, ComPtr<ID3D11Texture2D>& texture, const std::vector<uint8_t>& image, const DXGI_FORMAT& pixelFormat, const int& mipLevels, const int& arraySize)
	{
		// Staging Texture ����
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

		// Pixel Format�� �´� �� �ȼ� ���� ũ�� 
		// RGBA �� 8bit�� ����ϸ� uint8_t * 4(�Ϲ����� �̹���), �� 16bit��� uint16_t * 4(HDRI) 
		size_t pixelSize = GetPixelSize(pixelFormat);

		// CPU���� �̹��� ������ ����
		D3D11_MAPPED_SUBRESOURCE ms;
		context->Map(texture.Get(), NULL, D3D11_MAP_WRITE, NULL, &ms);
		uint8_t* pData = (uint8_t*)ms.pData; // uint8_t�� ���� 1�� �� (ex: R�� 1��)
		for (UINT h = 0; h < UINT(height); ++h) { 
			// GPU �޸𸮿� CPU �޸𸮰� 1��1�� �������� �ʱ⿡ ������ �� �پ� ����
			memcpy(&pData[h * ms.RowPitch], &image[h * width * pixelSize], width * pixelSize);
		}
		context->Unmap(texture.Get(), NULL);
	}

	void D3D11Utils::CreateTextureArray(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const std::vector<std::string>& filenames, Texture2D& texture)
	{
		// TODO: ��� �̹����� width�� height�� ���ٰ� ����
		std::vector<Image> imgs(filenames.size(), Image(L"Textures"));

		// ���Ϸκ��� �̹��� ���� ���� �о����
		for (size_t i = 0; i < filenames.size(); ++i)
			if (!imgs[i].Load(filenames[i]))
				throw std::exception();

		UINT size = UINT(filenames.size());

		// Texture2DArray�� ���� (�̶� �����͸� CPU�κ��� �������� ����)
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Width = UINT(imgs[0].GetWidth());
		desc.Height = UINT(imgs[0].GetHeight());
		desc.MipLevels = 0; // Mipmap Level �ִ�
		desc.ArraySize = size; // Texture Array�̹Ƿ� ����� Texture ����
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT; // Staging Texture�κ��� ���� ����
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS; // MipMap ���

		// SUBRESOURCE_DATA�� �迭
		//std::vector<D3D11_SUBRESOURCE_DATA> initData(size);
		//size_t offset = 0;
		//size_t pixelSize = GetPixelSize(desc.Format);
		//for (auto& i : initData) {
		//	// ������ �̹����� �����ϴ� ������
		//	//i.pSysMem = imageArray.data() + offset;
		//	i.pSysMem = img[offset++].GetImage().data();
		//	// ������ �ϳ��� ������ ũ��
		//	i.SysMemPitch = desc.Width * pixelSize;
		//	// 2D���� ������� ������ 3D���ʹ� ���Ǵ� �� ���� ũ��
		//	i.SysMemSlicePitch = desc.Width * desc.Height * pixelSize; // �̹��� �ϳ��� ������ ũ��
		//	//offset += i.SysMemSlicePitch; // ���� �̹����� �������� �˱� ���� offset
		//}
		//ThrowIfFailed(device->CreateTexture2D(&desc, initData.data(), texture.GetAddressOf()));

		// �Ϲ����� Texture2D�� srv desc�� ���� ���ص� �ǳ� Texture2D�� Arrayó�� ����ϱ� ���ؼ� ����
		//D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		//ZeroMemory(&srvDesc, sizeof(srvDesc));
		//srvDesc.Format = desc.Format;
		//// Array�� ����ϰڴٴ� ���� �ٽ�
		//srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		//srvDesc.Texture2DArray.MostDetailedMip = 0;
		//srvDesc.Texture2DArray.MipLevels = desc.MipLevels;
		//srvDesc.Texture2DArray.FirstArraySlice = 0;
		//// ��ŭ ū Array�� ����Ұ��� �������ִ� �ٽ�
		//srvDesc.Texture2DArray.ArraySize = desc.ArraySize;
		//ThrowIfFailed(device->CreateShaderResourceView(texture.Get(), &srvDesc, textureSRV.GetAddressOf()));

		// �ʱ� ������ ���� Texture ����
		ThrowIfFailed(device->CreateTexture2D(&desc, nullptr, texture.GetAddressOfTexture()));

		// StagingTexture�� ���� �ϳ��� ����
		for (size_t i = 0; i < imgs.size(); ++i) {
			// StagingTexture�� Texture2DArray�� �ƴ϶� Texture2D
			ComPtr<ID3D11Texture2D> stagingTexture;
			CreateStagingTexture(device, context, imgs[i].GetWidth(), imgs[i].GetHeight(), stagingTexture, imgs[i].GetImage());

			// Staging Texture�� Texture �迭�� �ش� ��ġ�� ����
			UINT subresourceIndex = D3D11CalcSubresource(0, UINT(i), desc.MipLevels);
			context->CopySubresourceRegion(texture.GetTexture(), subresourceIndex, 0, 0, 0, stagingTexture.Get(), 0, nullptr);
		}

		ThrowIfFailed(device->CreateShaderResourceView(texture.GetTexture(), nullptr, texture.GetAddressOfSRV()));
		
		context->GenerateMips(texture.GetSRV());
	}

	void D3D11Utils::CreateMetallicRoughnessTexture(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const std::string& metallicFilename, const std::string& roughnessFilename, Texture2D& texture)
	{
		// GLTF�� Metallic�� Roughness�� �̹� ������ Texture�� ���
		if (!metallicFilename.empty() && (metallicFilename == roughnessFilename))
			CreateTexture(device, context, metallicFilename, false, texture);
		// �ٸ� Format(fbx ��)�� Metallic, Roughness�� ���� ���� ���� ���ļ� �ϳ��� Texture�� ������ֱ�
		else {
			// ������ ������ ��� ���� �о �����ֱ�
			
			// Image Ŭ������ Ȱ���ϱ� ���ؼ� �� �̹������� ���� 4ä�η� ��ȯ �� 
			// �ٽ� 3ä�η� ��ġ�� ������� ����
			Image mImage(L"metallic"); // metallic
			Image rImage(L"roughness"); // Roughness

			int width = 0, height = 0;
			// ���࿡ �� �� �ϳ��� ���� ��쵵 �����ϱ� ���ؼ� ���� ���ϸ� Ȯ��
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

			// �� �̹����� �ػ󵵰� ���ٰ� ����
			if (!metallicFilename.empty() && !roughnessFilename.empty()) {
				assert(mImage.GetWidth() == rImage.GetWidth());
				assert(mImage.GetHeight() == rImage.GetHeight());
			}

			const std::vector<uint8_t>& metallic = mImage.GetImage();
			const std::vector<uint8_t>& roughness = rImage.GetImage();

			std::vector<uint8_t> combinedImage(mImage.GetSize());
			std::fill(combinedImage.begin(), combinedImage.end(), 0);

			// GLTF���� G�� Roughness, B�� Metallic���� ���Ǳ⿡ ���Ͻ����ֱ�
			size_t pixelSize = size_t(width * height);
			for (size_t i = 0; i < pixelSize; ++i) {
				// Roughness Texture�� �ִٸ�
				if (rImage.GetSize())
					combinedImage[4 * i + 1] = roughness[4 * i]; // Green = Roughness
				// metallic Texture�� �ִٸ�
				if (mImage.GetSize()) 
					combinedImage[4 * i + 2] = metallic[4 * i]; // Blue = Metalness
			}

			CreateTextureHelper(device, context, width, height, combinedImage, DXGI_FORMAT_R8G8B8A8_UNORM, texture);
		}
	}

	void D3D11Utils::CreateMetallicRoughnessTexture(ID3D11Device* device, ID3D11DeviceContext* context, const std::string& metallicFilename, const std::string& roughnessFilename, Texture2D& texture)
	{
		// 1. Image2�� ����Ͽ� �ؽ�ó �ε� �� ���� ���� (R8G8B8A8_UNORM)
		Image2 metalImg;
		bool hasMetal = !metallicFilename.empty() && metalImg.Load(metallicFilename);
		if (hasMetal) metalImg.Convert(DXGI_FORMAT_R8G8B8A8_UNORM);

		Image2 roughImg;
		bool hasRough = !roughnessFilename.empty() && roughImg.Load(roughnessFilename);
		if (hasRough) roughImg.Convert(DXGI_FORMAT_R8G8B8A8_UNORM);

		// �� �� ������ �������� ����
		if (!hasMetal && !hasRough) return;

		// 2. ���� �ؽ�ó ũ�� ���� (�� �� ū ������ ����)
		size_t width = 1;
		size_t height = 1;
		if (hasMetal) {
			width = metalImg.GetWidth();
			height = metalImg.GetHeight();
		}
		if (hasRough) {
			width = std::max(width, (size_t)roughImg.GetWidth());
			height = std::max(height, (size_t)roughImg.GetHeight());
		}

		// 3. ũ�Ⱑ �ٸ��ٸ� �������� (Image2::Resize Ȱ��)
		if (hasMetal && (metalImg.GetWidth() != width || metalImg.GetHeight() != height)) {
			metalImg.Resize(width, height);
		}
		if (hasRough && (roughImg.GetWidth() != width || roughImg.GetHeight() != height)) {
			roughImg.Resize(width, height);
		}

		// 4. ���� ����� ���� ScratchImage ����
		DirectX::ScratchImage resultImg;
		HRESULT hr = resultImg.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1);
		ThrowIfFailed(hr);

		// ������ ������ ȹ��
		const DirectX::Image* resImage = resultImg.GetImages();
		uint8_t* destPtr = resImage->pixels;
		size_t destPitch = resImage->rowPitch;

		const uint8_t* metalPtr = hasMetal ? metalImg.GetBuffer().GetImages()->pixels : nullptr;
		size_t metalPitch = hasMetal ? metalImg.GetBuffer().GetImages()->rowPitch : 0;

		const uint8_t* roughPtr = hasRough ? roughImg.GetBuffer().GetImages()->pixels : nullptr;
		size_t roughPitch = hasRough ? roughImg.GetBuffer().GetImages()->rowPitch : 0;

		// 5. �ȼ� ��ȸ�ϸ� ä�� ����
		// glTF PBR Standard: R=AO, G=Roughness, B=Metallic
		for (size_t y = 0; y < height; ++y)
		{
			uint32_t* destRow = reinterpret_cast<uint32_t*>(destPtr + y * destPitch);
			const uint32_t* metalRow = metalPtr ? reinterpret_cast<const uint32_t*>(metalPtr + y * metalPitch) : nullptr;
			const uint32_t* roughRow = roughPtr ? reinterpret_cast<const uint32_t*>(roughPtr + y * roughPitch) : nullptr;

			for (size_t x = 0; x < width; ++x)
			{
				float m = 0.0f; // Default Metallic (��ݼ�)
				float r = 1.0f; // Default Roughness (��ħ)

				// �� �ؽ�ó�� Red ä�� ���� ������ (��� �̹������ ����)
				if (metalRow) {
					uint32_t pixel = metalRow[x];
					m = (pixel & 0xFF) / 255.0f;
				}
				if (roughRow) {
					uint32_t pixel = roughRow[x];
					r = (pixel & 0xFF) / 255.0f;
				}

				// ä�� ����
				uint8_t ao = 255; // AO ������ �����Ƿ� 1.0 (Full White)
				uint8_t g = static_cast<uint8_t>(r * 255.0f); // Roughness
				uint8_t b = static_cast<uint8_t>(m * 255.0f); // Metallic
				uint8_t a = 255;

				// Little Endian Packing (A B G R ����)
				// 0xAABBGGRR
				uint32_t packed = (a << 24) | (b << 16) | (g << 8) | ao;
				destRow[x] = packed;
			}
		}

		// 6. �ؽ�ó �� SRV ���� (Mipmap ����)
		CreateTexture(device, context, resultImg, DXGI_FORMAT_R8G8B8A8_UNORM, texture);
	}

	void D3D11Utils::CreateTexturesFromGLTFCombined(ID3D11Device* device, ID3D11DeviceContext* context, const std::string& gltfTexturePath, Texture2D& outMetallicTex, Texture2D& outRoughnessTex)
	{
		// 1. ����(Combined) �̹��� �ε�
		Image2 sourceImg;
		if (!sourceImg.Load(gltfTexturePath))
			return;

		// ������ ó���� ���� �ϱ� ���� RGBA �������� ��ȯ
		sourceImg.Convert(DXGI_FORMAT_R8G8B8A8_UNORM);

		size_t width = sourceImg.GetWidth();
		size_t height = sourceImg.GetHeight();

		// 2. �и��� �����͸� ���� ScratchImage 2�� ���� (1ä�� ���� ���: R8_UNORM)
		DirectX::ScratchImage metalScratch;
		DirectX::ScratchImage roughScratch;

		ThrowIfFailed(metalScratch.Initialize2D(DXGI_FORMAT_R8_UNORM, width, height, 1, 1));
		ThrowIfFailed(roughScratch.Initialize2D(DXGI_FORMAT_R8_UNORM, width, height, 1, 1));

		// 3. �ȼ� ������ ������ ȹ��
		const uint8_t* srcPixels = sourceImg.GetBuffer().GetImages()->pixels;
		size_t srcPitch = sourceImg.GetBuffer().GetImages()->rowPitch;

		uint8_t* metalPixels = metalScratch.GetImages()->pixels;
		size_t metalPitch = metalScratch.GetImages()->rowPitch;

		uint8_t* roughPixels = roughScratch.GetImages()->pixels;
		size_t roughPitch = roughScratch.GetImages()->rowPitch;

		// 4. ä�� �и� (glTF: R=Occlusion, G=Roughness, B=Metallic)
		for (size_t y = 0; y < height; ++y)
		{
			const uint32_t* srcRow = reinterpret_cast<const uint32_t*>(srcPixels + y * srcPitch);
			uint8_t* metalRow = metalPixels + y * metalPitch;
			uint8_t* roughRow = roughPixels + y * roughPitch;

			for (size_t x = 0; x < width; ++x)
			{
				uint32_t pixel = srcRow[x];

				// Little Endian: 0xAABBGGRR
				// Green Channel = Roughness
				uint8_t g = (pixel >> 8) & 0xFF;
				// Blue Channel = Metallic
				uint8_t b = (pixel >> 16) & 0xFF;

				// ������ �ؽ�ó�� ���� (1ä���̹Ƿ� �� �״�� ����)
				roughRow[x] = g;
				metalRow[x] = b;
			}
		}

		// 5. ���� �ؽ�ó ���� (������ �ۼ��� CreateTexture �����ε� Ȱ��)
		CreateTexture(device, context, metalScratch, DXGI_FORMAT_R8_UNORM, outMetallicTex);
		CreateTexture(device, context, roughScratch, DXGI_FORMAT_R8_UNORM, outRoughnessTex);
	}

	void D3D11Utils::CreateTexture2DArray(ID3D11Device* device, UINT width, UINT height, UINT arraySize, bool useSRGB, ComPtr<ID3D11Texture2D>& outTexture, ComPtr<ID3D11ShaderResourceView>& outSRV)
	{
		DXGI_FORMAT pixelFormat = useSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM; // �Ϲ����� �̹��� ������ ������ uint8_t�̱⿡ R8G8B8A8_UNORM ���

		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = 0; // Mipmap Level �ִ�
		desc.ArraySize = arraySize; // Texture Array�̹Ƿ� ����� Texture ����
		desc.Format = pixelFormat;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT; // Staging Texture�κ��� ���� ����
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS; // MipMap ���

		// �ʱ� ������ ���� ����
		ThrowIfFailed(device->CreateTexture2D(&desc, nullptr, outTexture.GetAddressOf()));

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = desc.Format;
		// Array�� ����ϰڴٴ� ���� �ٽ�
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.MipLevels = -1;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		// ��ŭ ū Array�� ����Ұ��� �������ִ� �ٽ�
		srvDesc.Texture2DArray.ArraySize = desc.ArraySize;
		ThrowIfFailed(device->CreateShaderResourceView(outTexture.Get(), &srvDesc, outSRV.GetAddressOf()));
	}

	void D3D11Utils::UpdateTextureArraySlice(ID3D11DeviceContext* context, ID3D11Texture2D* textureArray, const Image2* image, UINT sliceIndex)
	{
		const auto& buffer = image->GetBuffer();
		const DirectX::Image* imgData = buffer.GetImages();

		D3D11_TEXTURE2D_DESC desc;
		textureArray->GetDesc(&desc);

		UINT mipLevels = desc.MipLevels;
		if (mipLevels == 0)
			// DX11���� MipLevels=0���� ������ ���� ������ Log2(max(w,h)) + 1 
			mipLevels = 1 + static_cast<UINT>(std::floor(std::log2(std::max(desc.Width, desc.Height))));

		UINT subresourceIndex = D3D11CalcSubresource(0, sliceIndex, mipLevels);
		
		// ������ ����
		context->UpdateSubresource(
			textureArray,
			subresourceIndex,
			nullptr,
			imgData->pixels,
			static_cast<UINT>(imgData->rowPitch),
			static_cast<UINT>(imgData->slicePitch)
		);
	}

	void D3D11Utils::CreateStagingTexture(ID3D11Device* device, ID3D11DeviceContext* context, const Image2* image, ComPtr<ID3D11Texture2D>& outStagingTexture)
	{
		// Image2 ������ DirectXTex ������ ��������
		const DirectX::ScratchImage& scratchImg = image->GetBuffer();
		const DirectX::Image* imgData = scratchImg.GetImages(); // ù ��° �̹���(Mip0, Slice0)

		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = static_cast<UINT>(imgData->width);
		desc.Height = static_cast<UINT>(imgData->height);
		desc.MipLevels = 1; // Staging�� ���� �뵵�̹Ƿ� MipMap ���ʿ� (���� 1�常)
		desc.ArraySize = 1;
		desc.Format = imgData->format;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_STAGING;
		desc.BindFlags = 0; // Staging�� Bind Flag ����
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;

		ThrowIfFailed(device->CreateTexture2D(&desc, nullptr, outStagingTexture.GetAddressOf()));

		// ������ ���� (Map/Unmap)
		D3D11_MAPPED_SUBRESOURCE ms;
		ThrowIfFailed(context->Map(outStagingTexture.Get(), 0, D3D11_MAP_WRITE, 0, &ms));

		// �޸� ���� (Row Pitch�� �����Ͽ� �� �پ� ����)
		const uint8_t* srcPtr = imgData->pixels;
		uint8_t* destPtr = static_cast<uint8_t*>(ms.pData);

		// ������ ���̸�ŭ �ݺ�
		for (size_t h = 0; h < imgData->height; ++h)
		{
			// min(Source Pitch, Dest Pitch) ��ŭ �����ؾ� ������
			// ���� DirectXTex�� �ε��ϸ� ������ ���� Pitch�� ���������, Dest�� �� Ŭ �� ����
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

	void D3D11Utils::CreateTexture3D(
		ID3D11Device* device,
		UINT width, UINT height, UINT depth,
		DXGI_FORMAT format,
		const void* initData,
		ComPtr<ID3D11Texture3D>& outTexture,
		ComPtr<ID3D11ShaderResourceView>& outSRV)
	{
		size_t pixelSize = GetPixelSize(format);

		D3D11_TEXTURE3D_DESC desc = {};
		desc.Width = width;
		desc.Height = height;
		desc.Depth = depth;
		desc.MipLevels = 1;
		desc.Format = format;
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA subData = {};
		subData.pSysMem = initData;
		subData.SysMemPitch = static_cast<UINT>(width * pixelSize);
		subData.SysMemSlicePitch = static_cast<UINT>(width * height * pixelSize);

		HRESULT hr = device->CreateTexture3D(&desc, &subData, outTexture.GetAddressOf());
		if (FAILED(hr)) {
			std::cout << "[D3D11Utils] Failed to create Texture3D" << std::endl;
			return;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
		srvDesc.Texture3D.MostDetailedMip = 0;
		srvDesc.Texture3D.MipLevels = 1;

		hr = device->CreateShaderResourceView(outTexture.Get(), &srvDesc, outSRV.GetAddressOf());
		if (FAILED(hr)) {
			std::cout << "[D3D11Utils] Failed to create Texture3D SRV" << std::endl;
			return;
		}
	}

	void D3D11Utils::CreateTexture(ID3D11Device* device, UINT width, UINT height, DXGI_FORMAT format, const void* initData, Texture2D& outTexture)
	{
		size_t pixelSize = GetPixelSize(format);

		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;          
		desc.Format = format;
		desc.SampleDesc.Count = 1; 
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA subData = {};
		subData.pSysMem = initData;
		subData.SysMemPitch = static_cast<UINT>(width * pixelSize);
		subData.SysMemSlicePitch = static_cast<UINT>(width * height * pixelSize);

		HRESULT hr = device->CreateTexture2D(&desc, &subData, outTexture.GetAddressOfTexture());
		if (FAILED(hr)) {
			std::cout << "[D3D11Utils] Failed to create Texture3D" << std::endl;
			return;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;

		hr = device->CreateShaderResourceView(outTexture.GetTexture(), &srvDesc, outTexture.GetAddressOfSRV());
		if (FAILED(hr)) {
			std::cout << "[D3D11Utils] Failed to create Texture3D SRV" << std::endl;
			return;
		}
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
		// Structured Buffer ����
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
		
		// SRV ����
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = numElements;
		ThrowIfFailed(device->CreateShaderResourceView(buffer.Get(), &srvDesc, srv.GetAddressOf()));

		// UAV ����
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = numElements;
		uavDesc.Buffer.Flags = 0;
		ThrowIfFailed(device->CreateUnorderedAccessView(buffer.Get(), &uavDesc, uav.GetAddressOf()));
	}

	void D3D11Utils::CreateStructuredBufferSRV(ID3D11Device* device, const UINT numElements, const UINT elementSize, const void* initData, ComPtr<ID3D11Buffer>& buffer, ComPtr<ID3D11ShaderResourceView>& srv)
	{
		// Structured Buffer ����
		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.ByteWidth = numElements * elementSize;
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
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

		// SRV ����
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = numElements;
		ThrowIfFailed(device->CreateShaderResourceView(buffer.Get(), &srvDesc, srv.GetAddressOf()));
	}

	void D3D11Utils::CreateStagingBuffer(ID3D11Device* device, const UINT numElements, const UINT elementSize, const void* initData, ComPtr<ID3D11Buffer>& buffer)
	{
		// StagingBuffer ����
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
	void D3D11Utils::CreateAppendBuffer(ID3D11Device* device, const UINT numElements, const UINT elementSize, const void* initData, ComPtr<ID3D11Buffer>& buffer, ComPtr<ID3D11ShaderResourceView>& srv, ComPtr<ID3D11UnorderedAccessView>& uav, ComPtr<ID3D11UnorderedAccessView>& rwUav)
	{
		// Structured Buffer ����
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

		// SRV ����
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = numElements;
		ThrowIfFailed(device->CreateShaderResourceView(buffer.Get(), &srvDesc, srv.GetAddressOf()));

		// UAV ����
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = numElements;
		uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_APPEND;
		ThrowIfFailed(device->CreateUnorderedAccessView(buffer.Get(), &uavDesc, uav.GetAddressOf()));

		uavDesc.Buffer.Flags = 0; // �÷��� ����! (RWStructuredBuffer ȣȯ)
		ThrowIfFailed(device->CreateUnorderedAccessView(buffer.Get(), &uavDesc, rwUav.GetAddressOf()));
	}

	void D3D11Utils::CreateIndirectBuffer(ID3D11Device* device, UINT byteWidth, UINT argCount, const void* initData, ComPtr<ID3D11Buffer>& buffer, ComPtr<ID3D11UnorderedAccessView>& uav)
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

		// UAV ���� (R32_UINT ���� ���)
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_R32_UINT; // uint�� �б� ���� R32_UINT ���
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = byteWidth / argCount; // UINT ���� (Byte / 4)
		uavDesc.Buffer.Flags = 0;

		ThrowIfFailed(device->CreateUnorderedAccessView(buffer.Get(), &uavDesc, uav.GetAddressOf()));
	}

	void D3D11Utils::CreateUnifiedIndirectBuffer(ID3D11Device* device, UINT arraySize, UINT elemSize, UINT argCount, const void* initData, ComPtr<ID3D11Buffer>& buffer, ComPtr<ID3D11UnorderedAccessView>& uav)
	{
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = arraySize * elemSize;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS | D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS; // Indirect Draw + Raw View
		desc.StructureByteStride = 0;

		if (initData) {
			D3D11_SUBRESOURCE_DATA data = {};
			data.pSysMem = initData;
			ThrowIfFailed(device->CreateBuffer(&desc, &data, buffer.GetAddressOf()));
		}
		else {
			ThrowIfFailed(device->CreateBuffer(&desc, NULL, buffer.GetAddressOf()));
		}

		// UAV ���� (R32_UINT ���� ���)
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_R32_UINT; // uint�� �б� ���� R32_UINT ���
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = arraySize * argCount;
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