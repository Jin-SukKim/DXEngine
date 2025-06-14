#include "pch.h"
#include "D3D11Utils.h"
#include "Image.h"
#include "Texture2D.h"

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

	void D3D11Utils::CreateTexture(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const std::string& filename, Texture2D& texture)
	{
		Image img(L"Image");

		std::string ext(filename.end() - 3, filename.end());
		std::transform(ext.begin(), ext.end(), ext.begin(), std::tolower);

		DXGI_FORMAT pixelFormat = DXGI_FORMAT_R8G8B8A8_UNORM; // 일반적인 이미지 파일의 형식은 uint8_t이기에 R8G8B8A8_UNORM 사용
		// image의 확장자가 exr이라면 HDRI란 의미
		if (ext == "exr") {
			if (!img.LoadExr(filename, pixelFormat)) throw std::exception();
		}
		else 
			if (!img.Load(filename)) throw std::exception();
		
		// Staging Texture 만들고 CPU에서 이미지를 복사
		ComPtr<ID3D11Texture2D> stagingTexture;
		CreateStagingTexture(device, context, img.GetWidth(), img.GetHeight(), stagingTexture, img.GetImage());

		// Texture 설정
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = img.GetWidth();
		desc.Height = img.GetHeight();
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

	void D3D11Utils::CreateStagingTexture(ComPtr<ID3D11Device>& device, const int& width, const int& height, ComPtr<ID3D11Texture2D>& texture, const DXGI_FORMAT pixelFormat)
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

	void D3D11Utils::CreateStagingTexture(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const int& width, const int& height, ComPtr<ID3D11Texture2D>& texture, const std::vector<uint8_t>& image, const int& mipLevels, const int& arraySize, const DXGI_FORMAT pixelFormat)
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

		// CPU에서 이미지 데이터 복사
		D3D11_MAPPED_SUBRESOURCE ms;
		context->Map(texture.Get(), NULL, D3D11_MAP_WRITE, NULL, &ms);
		uint8_t* pData = (uint8_t*)ms.pData; // uint8_t는 색깔 1개 값 (ex: R값 1개)
		for (UINT h = 0; h < UINT(height); ++h) { 
			// GPU 메모리와 CPU 메모리가 1대1로 대응되지 않기에 가로줄 한 줄씩 복사
			memcpy(&pData[h * ms.RowPitch], &image[h * width * 4], width * sizeof(uint8_t) * 4);
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
		//for (auto& i : initData) {
		//	// 각각의 이미지가 시작하는 시작점
		//	//i.pSysMem = imageArray.data() + offset;
		//	i.pSysMem = img[offset++].GetImage().data();
		//	// 가로줄 하나의 데이터 크기
		//	i.SysMemPitch = desc.Width * sizeof(uint8_t) * 4;
		//	// 2D에선 사용하지 않으나 3D부터는 사용되는 한 면의 크기
		//	i.SysMemSlicePitch = desc.Width * desc.Height * sizeof(uint8_t) * 4; // 이미지 하나의 데이터 크기
		//	//offset += i.SysMemSlicePitch; // 다음 이미지의 시작점을 알기 위한 offset
		//}
		//ThrowIfFailed(device->CreateTexture2D(&desc, initData.data(), texture.GetAddressOf()));

		// 일반적인 Texture2D는 srv desc를 설정 안해도 되나 Texture2D를 Array처럼 상6ㅛㅇ하기 위해선 설정
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


	void D3D11Utils::CopyFromStagingTexture(ComPtr<ID3D11DeviceContext>& context, const ComPtr<ID3D11Texture2D>& texture, UINT size, void* dest)
	{
		D3D11_MAPPED_SUBRESOURCE ms;
		context->Map(texture.Get(), NULL, D3D11_MAP_READ, NULL, &ms);
		memcpy(dest, ms.pData, size);
		context->Unmap(texture.Get(), NULL);
	}
}