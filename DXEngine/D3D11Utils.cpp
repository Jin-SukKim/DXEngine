#include "pch.h"
#include "D3D11Utils.h"
#include "Image.h"
#include "Texture2D.h"

#include <directxtk/DDSTextureLoader.h>
#include "stb_image.h"
#include "stb_image_write.h"

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

	void D3D11Utils::CreateTexture(ComPtr<ID3D11Device>& device, const std::string& filename, Texture2D& texture)
	{
		Image img(L"Image");
		if (!img.Load(filename))
			throw std::exception();

		// Texture 설정
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = img.GetWidth();
		desc.Height = img.GetHeight();
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 일반적인 이미지 파일의 형식은 uint8_t이기에 R8G8B8A8_UNORM 사용
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT; 
		// Shader Resource View로 사용
		desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0; // No CPU Access

		// 어떤 데이터로 초기화할지 설정
		D3D11_SUBRESOURCE_DATA initData;
		initData.pSysMem = img.GetImage().data();
		initData.SysMemPitch = desc.Width * sizeof(uint8_t) * img.GetChannels();
		initData.SysMemSlicePitch = 0; // 데이터가 배열인 경우 사용

		ThrowIfFailed(device->CreateTexture2D(&desc, &initData, texture.GetAddressOfTexture()));
		ThrowIfFailed(device->CreateShaderResourceView(texture.GetTexture(), nullptr, texture.GetAddressOfSRV()));
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

	void D3D11Utils::CreateImageFilterTexture(ComPtr<ID3D11Device>& device, int width, int height, Texture2D& texture)
	{
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = desc.ArraySize = 1;
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

	void D3D11Utils::CreateStagingTexture(ComPtr<ID3D11Device>& device, const int& width, const int& height, ComPtr<ID3D11Texture2D>& texture, const DXGI_FORMAT pixelFormat)
	{
		// Staginge Texture 생성
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.BindFlags = 0;
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = desc.ArraySize = 1;
		desc.Format = pixelFormat;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_STAGING; // GPU->CPU로 데이터를 보낼 용도
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE; // CPU에서 접근

		ThrowIfFailed(device->CreateTexture2D(&desc, NULL, texture.GetAddressOf()));
	}

	void D3D11Utils::CreateTextureArray(ComPtr<ID3D11Device>& device, const std::vector<std::string>& filenames, ComPtr<ID3D11Texture2D>& texture, ComPtr<ID3D11ShaderResourceView>& textureSRV)
	{
		if (filenames.empty())
			return;

		// TODO: 모든 이미지의 width와 height이 같다고 가정
		
		// 파일로부터 이미지 여러 개를 읽어들임
		int width = 0, height = 0;
		std::vector<std::vector<uint8_t>> imageArray;
		for (const std::string &f : filenames) {
			std::cout << f << std::endl;

			std::vector<uint8_t> image;
			ReadImage(f, image, width, height);
			imageArray.emplace_back(image);
		}

		UINT size = UINT(filenames.size());

		// Texture2DArray를 생성 (이때 데이터를 CPU로부터 복사하지 않음)
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Width = UINT(width);
		desc.Height = UINT(height);
		desc.MipLevels = 1; // Mipmap Level 최대
		desc.ArraySize = size; // Texture Array이므로 사용할 Texture 개수
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT; // Staging Texture로부터 복사 가능
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		//desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS; // MipMap 사용

		// SUBRESOURCE_DATA의 배열
		std::vector<D3D11_SUBRESOURCE_DATA> initData(size);
		size_t offset = 0;
		for (auto& i : initData) {
			// 각각의 이미지가 시작하는 시작점
			//i.pSysMem = imageArray.data() + offset;
			i.pSysMem = imageArray[offset++].data();
			// 가로줄 하나의 데이터 크기
			i.SysMemPitch = desc.Width * sizeof(uint8_t) * 4;
			// 2D에선 사용하지 않으나 3D부터는 사용되는 한 면의 크기
			i.SysMemSlicePitch = desc.Width * desc.Height * sizeof(uint8_t) * 4; // 이미지 하나의 데이터 크기
			//offset += i.SysMemSlicePitch; // 다음 이미지의 시작점을 알기 위한 offset
		}

		ThrowIfFailed(device->CreateTexture2D(&desc, initData.data(), texture.GetAddressOf()));
		
		// 일반적인 Texture2D는 srv desc를 설정 안해도 되나 Texture2D를 Array처럼 상6ㅛㅇ하기 위해선 설정
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = desc.Format;
		// Array로 사용하겠다는 설정 핵심
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.MipLevels = desc.MipLevels;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		// 얼만큼 큰 Array를 사용할건지 설정해주는 핵심
		srvDesc.Texture2DArray.ArraySize = desc.ArraySize;

		ThrowIfFailed(device->CreateShaderResourceView(texture.Get(), &srvDesc, textureSRV.GetAddressOf()));
	}


	void D3D11Utils::CopyFromStagingTexture(ComPtr<ID3D11DeviceContext>& context, const ComPtr<ID3D11Texture2D>& texture, UINT size, void* dest)
	{
		D3D11_MAPPED_SUBRESOURCE ms;
		context->Map(texture.Get(), NULL, D3D11_MAP_READ, NULL, &ms);
		memcpy(dest, ms.pData, size);
		context->Unmap(texture.Get(), NULL);
	}

	void D3D11Utils::ReadImage(const std::string& filename, std::vector<uint8_t>& image, int& width, int& height)
	{
		int channels;
		unsigned char* img = stbi_load(filename.c_str(), &width, &height, &channels, 0);

		std::cout << "ReadImage() " << filename << " " << width << " " << height << " " << channels << std::endl;

		// 4채널로 만들어서 복사
		image.resize(width * height * 4);

		if (channels == 0) {
			std::cout << "Cannot read " << channels << " channels" << std::endl;
			stbi_image_free(img);
		}

		for (size_t i = 0; i < width * height; ++i) {
			for (size_t c = 0; c < channels; ++c)
				image[4 * i + c] = img[i * channels + c];
			for (size_t c = channels; c < 4; ++c)
				image[4 * i + c] = 255;
		}
	}

}