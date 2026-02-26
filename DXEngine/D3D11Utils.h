#pragma once
#include "Image2.h"

namespace DE {
	class Image2;

	class Texture2D;
	inline void ThrowIfFailed(HRESULT hr) {
		if (FAILED(hr)) {
			throw std::exception();
		}
	}
	class D3D11Utils
	{
	public:
		template<typename T_VERTEX>
		static void CreateVertexBuffer(ComPtr<ID3D11Device>& device, const std::vector<T_VERTEX>& vertices, ComPtr<ID3D11Buffer>& vertexBuffer) {
			D3D11_BUFFER_DESC desc = {};
			ZeroMemory(&desc, sizeof(desc));
			desc.Usage = D3D11_USAGE_DEFAULT; // GPU read/write
			desc.ByteWidth = UINT(sizeof(T_VERTEX) * vertices.size());
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			desc.CPUAccessFlags = 0; // 0 if no CPU access if necessary
			desc.StructureByteStride = sizeof(T_VERTEX);

			D3D11_SUBRESOURCE_DATA bufferData = { 0 };
			bufferData.pSysMem = vertices.data(); 
			bufferData.SysMemPitch = 0;
			bufferData.SysMemSlicePitch = 0;

			ThrowIfFailed(device->CreateBuffer(&desc, &bufferData, vertexBuffer.GetAddressOf()));
		}

		static void CreateIndexBuffer(ComPtr<ID3D11Device>& device, const std::vector<uint32_t>& indices, ComPtr<ID3D11Buffer>& indexBuffer);
		
		template<typename T_VERTEX>
		static void CreateVertexBufferPool(ComPtr<ID3D11Device>& device, const UINT& maxCount, ComPtr<ID3D11Buffer>& vertexBuffer) {
			D3D11_BUFFER_DESC desc = {};
			desc.Usage = D3D11_USAGE_DEFAULT; // UpdateSubresource  
			desc.ByteWidth = UINT(sizeof(T_VERTEX) * maxCount); 
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			desc.CPUAccessFlags = 0; // CPU (UpdateSubresource)
			desc.StructureByteStride = sizeof(T_VERTEX);

			ThrowIfFailed(device->CreateBuffer(&desc, nullptr, vertexBuffer.GetAddressOf()));
		}

		static void CreateIndexBufferPool(ComPtr<ID3D11Device>& device, const UINT& maxCount, ComPtr<ID3D11Buffer>& indexBuffer)
		{
			D3D11_BUFFER_DESC desc = {};
			// IMMUTABLE -> DEFAULT
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.ByteWidth = UINT(sizeof(uint32_t) * maxCount);
			desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			desc.CPUAccessFlags = 0;
			desc.StructureByteStride = sizeof(uint32_t);

			device->CreateBuffer(&desc, nullptr, indexBuffer.GetAddressOf());
		}

		template<typename T_CONSTANT>
		static void CreateConstantBuffer(ID3D11Device* device, const T_CONSTANT& constantData, ComPtr<ID3D11Buffer>& constantBuffer) {
			D3D11_BUFFER_DESC desc = {};
			ZeroMemory(&desc, sizeof(desc));
			desc.Usage = D3D11_USAGE_DYNAMIC; 
			desc.ByteWidth = UINT(sizeof(constantData));
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.StructureByteStride = 0;

			D3D11_SUBRESOURCE_DATA bufferData = { 0 };
			bufferData.pSysMem = &constantData;
			bufferData.SysMemPitch = 0;
			bufferData.SysMemSlicePitch = 0;

			ThrowIfFailed(device->CreateBuffer(&desc, &bufferData, constantBuffer.GetAddressOf()));
		}

		// Vertex Shader InputLayout 
		static void CreateVSAndIL(ComPtr<ID3D11Device>& device, const std::wstring& filename, const std::vector<D3D11_INPUT_ELEMENT_DESC>& inputElements, ComPtr<ID3D11VertexShader>& vertexShader, ComPtr<ID3D11InputLayout>& inputLayout);
		// Pixel Shader 
		static void CreatePS(ComPtr<ID3D11Device>& device, const std::wstring& filename, ComPtr<ID3D11PixelShader>& pixelShader);
		// Geometry Shader 
		static void CreateGS(ComPtr<ID3D11Device>& device, const std::wstring& filename, ComPtr<ID3D11GeometryShader>& geometryShader);
		// Hull Shader  (Control Points/Patch Shader)
		static void CreateHS(ComPtr<ID3D11Device>& device, const std::wstring& filename, ComPtr<ID3D11HullShader>& hullShader);
		// Domain Shader 
		static void CreateDS(ComPtr<ID3D11Device>& device, const std::wstring& filename, ComPtr<ID3D11DomainShader>& domainShader);


		// Usage Dynamic Buffer GPU GPU
		template<typename T_DATA>
		static void UpdateBuffer(ComPtr<ID3D11DeviceContext>& context, const T_DATA& bufferData, ComPtr<ID3D11Buffer>& buffer) {
			D3D11_MAPPED_SUBRESOURCE ms;
			context->Map(buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
			memcpy(ms.pData, &bufferData, sizeof(bufferData));
			context->Unmap(buffer.Get(), 0);
		}

		// Texture2D 
		static void CreateTexture(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const std::string& filename, const bool usSRGB, Texture2D& texture);
		// ScratchImage
		static void CreateTexture(
			ID3D11Device* device,
			ID3D11DeviceContext* context,
			const DirectX::ScratchImage& image, // Image2*  ScratchImage& 
			const DXGI_FORMAT& format,
			Texture2D& texture
		);

		// Image2 (Wrapper)
		static void CreateTexture(
			ID3D11Device* device,
			ID3D11DeviceContext* context,
			const Image2* image,
			const DXGI_FORMAT& format,
			Texture2D& texture
		);
		// Resource Texture Texture, SRV, RTV 
		static void CreateTexture(ComPtr<ID3D11Device>& device, const ComPtr<ID3D11Texture2D>& resource, Texture2D& texture);
		static void CreateTexture(ComPtr<ID3D11Device>& device, const D3D11_TEXTURE2D_DESC& desc, Texture2D& texture);
		static void CreateTextureHelper(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const int& width, const int& height, const std::vector<uint8_t>& image, const DXGI_FORMAT& pixelFormat, DE::Texture2D& texture);
		// Post-Process Texture 
		static void CreateImageFilterTexture(ComPtr<ID3D11Device>& device, int width, int height, Texture2D& texture);
		// DDS Texture (isCubemap true Cubemap Texture, false Texture2D )
		static void CreateDDSTexture(ComPtr<ID3D11Device>& device, const std::wstring& filename, bool isCubeMap, ComPtr<ID3D11ShaderResourceView>& textureResourceView);
		// GPU CPU Staging Texture 
		static void CreateStagingTexture(ComPtr<ID3D11Device>& device, const int& width, const int& height, ComPtr<ID3D11Texture2D>& texture, const DXGI_FORMAT& pixelFormat = DXGI_FORMAT_R8G8B8A8_UNORM);
		// Mipmap Staging Texture 
		static void CreateStagingTexture(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const int& width, const int& height, ComPtr<ID3D11Texture2D>& texture, const std::vector<uint8_t>& image, const DXGI_FORMAT& pixelFormat = DXGI_FORMAT_R8G8B8A8_UNORM, const int& mipLevels = 1, const int& arraySize = 1);
		// Texture Array 
		static void CreateTextureArray(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const std::vector<std::string>& filenames, Texture2D& texture);
		// Metal Roughness Texture
		static void CreateMetallicRoughnessTexture(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const std::string& metallicFilename, const std::string& roughnessFilename, Texture2D& texture);
		static void CreateMetallicRoughnessTexture(ID3D11Device* device, ID3D11DeviceContext* context, const std::string& metallicFilename, const std::string& roughnessFilename, Texture2D& texture);
		static void CreateTexturesFromGLTFCombined(
			ID3D11Device* device,
			ID3D11DeviceContext* context,
			const std::string& gltfTexturePath,
			Texture2D& outMetallicTex,
			Texture2D& outRoughnessTex
		);
		// Texture2D Array 
		static void CreateTexture2DArray(ID3D11Device* device,
			UINT width, UINT height, UINT arraySize,
			bool useSRGB,
			ComPtr<ID3D11Texture2D>& outTexture,
			ComPtr<ID3D11ShaderResourceView>& outSRV);
		// Texture2D Array
		static void UpdateTextureArraySlice(
			ID3D11DeviceContext* context,
			ID3D11Texture2D* textureArray,
			const Image2* image,
			UINT sliceIndex);
		// Image2  Miamap  Stating Texture 
		static void CreateStagingTexture(ID3D11Device* device,
			ID3D11DeviceContext* context,
			const Image2* image,
			ComPtr<ID3D11Texture2D>& outStagingTexture);
		static void CopyFromStagingTexture(ComPtr<ID3D11DeviceContext>& context, const ComPtr<ID3D11Texture2D>& texture, UINT size, void* dest);

		// 3D Texture (Volume Texture) 생성
		static void CreateTexture3D(
			ID3D11Device* device,
			UINT width, UINT height, UINT depth,
			DXGI_FORMAT format,
			const void* initData,
			ComPtr<ID3D11Texture3D>& outTexture,
			ComPtr<ID3D11ShaderResourceView>& outSRV);

		static void CreateTexture(
			ID3D11Device* device,
			UINT width, UINT height,
			DXGI_FORMAT format,
			const void* initData,
			Texture2D& outTexture
			);

		// Pixel Format
		static size_t GetPixelSize(const DXGI_FORMAT& pixelFormat);


		// Particle System
		static void CreateStructuredBuffer(ID3D11Device* device, const UINT numElements, const UINT elementSize, const void* initData, ComPtr<ID3D11Buffer>& buffer, ComPtr<ID3D11ShaderResourceView>& srv, ComPtr<ID3D11UnorderedAccessView>& uav);
		static void CreateStructuredBufferSRV(ID3D11Device* device, const UINT numElements, const UINT elementSize, const void* initData, ComPtr<ID3D11Buffer>& buffer, ComPtr<ID3D11ShaderResourceView>& srv);
		static void CreateStagingBuffer(ID3D11Device* device, const UINT numElements, const UINT elementSize, const void* initData, ComPtr<ID3D11Buffer>& buffer);
		static void CopyToStagingBuffer(ID3D11DeviceContext* context, ID3D11Buffer* dest, UINT size, void* src);
		static void CopyFromStagingBuffer(ID3D11DeviceContext* context, void* dest, UINT size, ID3D11Buffer* src);
		static void CreateCS(ID3D11Device* device, const std::wstring& filename, ComPtr<ID3D11ComputeShader>& computeShader);
		static void CreateAppendBuffer(ID3D11Device* device, const UINT numElements, const UINT elementSize, const void* initData, ComPtr<ID3D11Buffer>& buffer, ComPtr<ID3D11ShaderResourceView>& srv, ComPtr<ID3D11UnorderedAccessView>& uav, ComPtr<ID3D11UnorderedAccessView>& rwUav);
		static void CreateIndirectBuffer(ID3D11Device* device, UINT byteWidth, UINT argCount, const void* initData, ComPtr<ID3D11Buffer>& buffer, ComPtr<ID3D11UnorderedAccessView>& uav);
		static void CreateUnifiedIndirectBuffer(ID3D11Device* device, UINT arraySize, UINT elemSize, UINT argCount, const void* initData, ComPtr<ID3D11Buffer>& buffer, ComPtr<ID3D11UnorderedAccessView>& uav);
		static void CreateBuffer(ID3D11Device* device, const UINT elementSize, const void* initData, DXGI_FORMAT format, ComPtr<ID3D11Buffer>& buffer, ComPtr<ID3D11ShaderResourceView>& srv);
		static void CreateLUTArray(ID3D11Device* device, UINT width, UINT height, UINT arraySize, ComPtr<ID3D11Texture2D>& outTexture, ComPtr<ID3D11ShaderResourceView>& outSRV);
	};
}