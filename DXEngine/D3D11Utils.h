#pragma once
//#include "Texture2D.h"
#include "Image2.h"

namespace DE {
	class Image2;

	class Texture2D;
	inline void ThrowIfFailed(HRESULT hr) {
		if (FAILED(hr)) {
			// 디버깅할 때 여기에 breakpoint를 설정
			throw std::exception();
		}
	}
	class D3D11Utils
	{
	public:
		template<typename T_VERTEX>
		static void CreateVertexBuffer(ComPtr<ID3D11Device>& device, const std::vector<T_VERTEX>& vertices, ComPtr<ID3D11Buffer>& vertexBuffer) {
			// Buffer를 어떻게 쓸지 설정
			D3D11_BUFFER_DESC desc = {};
			ZeroMemory(&desc, sizeof(desc));
			desc.Usage = D3D11_USAGE_DEFAULT; // GPU read/write
			desc.ByteWidth = UINT(sizeof(T_VERTEX) * vertices.size()); // 배열 전체 크기
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			desc.CPUAccessFlags = 0; // 0 if no CPU access if necessary
			desc.StructureByteStride = sizeof(T_VERTEX); // 데이터 하나를 읽을때의 크기

			// CPU에서 GPU로 데이터를 보낼때 어떤 데이터를 어떤 형식으로 보낼지 설정
			D3D11_SUBRESOURCE_DATA bufferData = { 0 };
			bufferData.pSysMem = vertices.data(); // 데이터를 어디서부터 보내기 시작할지
			bufferData.SysMemPitch = 0;
			bufferData.SysMemSlicePitch = 0;

			// GPU에서 메모리 생성
			ThrowIfFailed(device->CreateBuffer(&desc, &bufferData, vertexBuffer.GetAddressOf()));
		}

		static void CreateIndexBuffer(ComPtr<ID3D11Device>& device, const std::vector<uint32_t>& indices, ComPtr<ID3D11Buffer>& indexBuffer);
		
		// ConstantBuffer는 보통 Update에서 값을 매 프레임 바꿔주므로 CPU에서 쓰기, GPU에서 읽기가 가능한 Buffer를 생성
		template<typename T_CONSTANT>
		static void CreateConstantBuffer(ID3D11Device* device, const T_CONSTANT& constantData, ComPtr<ID3D11Buffer>& constantBuffer) {
			D3D11_BUFFER_DESC desc = {};
			ZeroMemory(&desc, sizeof(desc));
			desc.Usage = D3D11_USAGE_DYNAMIC; // CPU에서 쓰기, GPU에서 읽기 가능
			desc.ByteWidth = UINT(sizeof(constantData));
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // CPU에서 쓰기 가능
			desc.StructureByteStride = 0; // 배열이 아니므로 0

			D3D11_SUBRESOURCE_DATA bufferData = { 0 };
			bufferData.pSysMem = &constantData;
			bufferData.SysMemPitch = 0;
			bufferData.SysMemSlicePitch = 0;

			ThrowIfFailed(device->CreateBuffer(&desc, &bufferData, constantBuffer.GetAddressOf()));
		}

		// Vertex Shader와 InputLayout 생성
		static void CreateVSAndIL(ComPtr<ID3D11Device>& device, const std::wstring& filename, const std::vector<D3D11_INPUT_ELEMENT_DESC>& inputElements, ComPtr<ID3D11VertexShader>& vertexShader, ComPtr<ID3D11InputLayout>& inputLayout);
		// Pixel Shader 생성
		static void CreatePS(ComPtr<ID3D11Device>& device, const std::wstring& filename, ComPtr<ID3D11PixelShader>& pixelShader);
		// Geometry Shader 생성
		static void CreateGS(ComPtr<ID3D11Device>& device, const std::wstring& filename, ComPtr<ID3D11GeometryShader>& geometryShader);
		// Hull Shader 생성 (Control Points로 이루어진 Patch를 다루는 Shader)
		static void CreateHS(ComPtr<ID3D11Device>& device, const std::wstring& filename, ComPtr<ID3D11HullShader>& hullShader);
		// Domain Shader 생성
		static void CreateDS(ComPtr<ID3D11Device>& device, const std::wstring& filename, ComPtr<ID3D11DomainShader>& domainShader);


		// Usage를 Dynamic으로 설정한 Buffer를 GPU에서 GPU 메모리로 데이터 복사
		template<typename T_DATA>
		static void UpdateBuffer(ComPtr<ID3D11DeviceContext>& context, const T_DATA& bufferData, ComPtr<ID3D11Buffer>& buffer) {
			D3D11_MAPPED_SUBRESOURCE ms;
			// GPU 메모리에 접근
			context->Map(buffer.Get(), NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
			// CPU 데이터를 GPU 메모리에 복사
			memcpy(ms.pData, &bufferData, sizeof(bufferData));
			// GPU 메모리 접근 해제
			context->Unmap(buffer.Get(), NULL);
			
		}

		// Texture2D 생성
		static void CreateTexture(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const std::string& filename, const bool usSRGB, Texture2D& texture);
		// [신규] ScratchImage를 직접 받는 버전 (실제 구현부)
		static void CreateTexture(
			ID3D11Device* device,
			ID3D11DeviceContext* context,
			const DirectX::ScratchImage& image, // Image2* 대신 ScratchImage& 사용
			const DXGI_FORMAT& format,
			Texture2D& texture
		);

		// [기존] Image2 포인터를 받는 버전 (Wrapper)
		static void CreateTexture(
			ID3D11Device* device,
			ID3D11DeviceContext* context,
			const Image2* image,
			const DXGI_FORMAT& format,
			Texture2D& texture
		);
		// Resource Texture의 설정을 가져와서 Texture, SRV, RTV 생성
		static void CreateTexture(ComPtr<ID3D11Device>& device, const ComPtr<ID3D11Texture2D>& resource, Texture2D& texture);
		static void CreateTexture(ComPtr<ID3D11Device>& device, const D3D11_TEXTURE2D_DESC& desc, Texture2D& texture);
		static void CreateTextureHelper(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const int& width, const int& height, const std::vector<uint8_t>& image, const DXGI_FORMAT& pixelFormat, DE::Texture2D& texture);
		// Post-Process 용 Texture 생성
		static void CreateImageFilterTexture(ComPtr<ID3D11Device>& device, int width, int height, Texture2D& texture);
		// DDS 파일로부터 Texture 생성 (isCubemap이 true면 Cubemap Texture, false면 Texture2D 생성)
		static void CreateDDSTexture(ComPtr<ID3D11Device>& device, const std::wstring& filename, bool isCubeMap, ComPtr<ID3D11ShaderResourceView>& textureResourceView);
		// GPU에서 CPU로 데이터를 복사해올 용도인 Staging Texture 생성
		static void CreateStagingTexture(ComPtr<ID3D11Device>& device, const int& width, const int& height, ComPtr<ID3D11Texture2D>& texture, const DXGI_FORMAT& pixelFormat = DXGI_FORMAT_R8G8B8A8_UNORM);
		// Mipmap을 위한 Staging Texture 생성
		static void CreateStagingTexture(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const int& width, const int& height, ComPtr<ID3D11Texture2D>& texture, const std::vector<uint8_t>& image, const DXGI_FORMAT& pixelFormat = DXGI_FORMAT_R8G8B8A8_UNORM, const int& mipLevels = 1, const int& arraySize = 1);
		// Texture Array 생성
		static void CreateTextureArray(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const std::vector<std::string>& filenames, Texture2D& texture);
		// Metal과 Roughness Texture를 하나의 Texture에서 사용하는 MetallicRoughness Texture 생성
		static void CreateMetallicRoughnessTexture(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const std::string& metallicFilename, const std::string& roughnessFilename, Texture2D& texture);
		static void CreateMetallicRoughnessTexture(ID3D11Device* device, ID3D11DeviceContext* context, const std::string& metallicFilename, const std::string& roughnessFilename, Texture2D& texture);
		// Texture2D Array 생성
		static void CreateTexture2DArray(ID3D11Device* device,
			UINT width, UINT height, UINT arraySize,
			bool useSRGB,
			ComPtr<ID3D11Texture2D>& outTexture,
			ComPtr<ID3D11ShaderResourceView>& outSRV);
		// Texture2D Array에 데이터를 복사
		static void UpdateTextureArraySlice(
			ID3D11DeviceContext* context,
			ID3D11Texture2D* textureArray,
			const Image2* image,
			UINT sliceIndex);
		// Image2를 사용해 Miamap을 위한 Stating Texture 생성
		static void CreateStagingTexture(ID3D11Device* device,
			ID3D11DeviceContext* context,
			const Image2* image,
			ComPtr<ID3D11Texture2D>& outStagingTexture);
		static void CopyFromStagingTexture(ComPtr<ID3D11DeviceContext>& context, const ComPtr<ID3D11Texture2D>& texture, UINT size, void* dest);

		// Pixel Format에 따라 Pixel 색상의 범위가 다르기 때문에 같은 uint8_t를 쓰지만 대신 데이터 범위가 다름
		static size_t GetPixelSize(const DXGI_FORMAT& pixelFormat);


		// Particle System
		static void CreateStructuredBuffer(ID3D11Device* device, const UINT numElements, const UINT elementSize, const void* initData, ComPtr<ID3D11Buffer>& buffer, ComPtr<ID3D11ShaderResourceView>& srv, ComPtr<ID3D11UnorderedAccessView>& uav);
		static void CreateStagingBuffer(ID3D11Device* device, const UINT numElements, const UINT elementSize, const void* initData, ComPtr<ID3D11Buffer>& buffer);
		static void CopyToStagingBuffer(ID3D11DeviceContext* context, ID3D11Buffer* dest, UINT size, void* src);
		static void CopyFromStagingBuffer(ID3D11DeviceContext* context, void* dest, UINT size, ID3D11Buffer* src);
		static void CreateCS(ID3D11Device* device, const std::wstring& filename, ComPtr<ID3D11ComputeShader>& computeShader);
		static void CreateAppendBuffer(ID3D11Device* device, const UINT numElements, const UINT elementSize, const void* initData, ComPtr<ID3D11Buffer>& buffer, ComPtr<ID3D11ShaderResourceView>& srv, ComPtr<ID3D11UnorderedAccessView>& uav, ComPtr<ID3D11UnorderedAccessView>& rwUav);
		static void CreateIndirectBuffer(ID3D11Device* device, UINT byteWidth, UINT argCount, const void* initData, ComPtr<ID3D11Buffer>& buffer, ComPtr<ID3D11UnorderedAccessView>& uav);
		static void CreateUnifiedIndirectBuffer(ID3D11Device* device, UINT arraySize, UINT elemSize, UINT argCount, const void* initData, ComPtr<ID3D11Buffer>& buffer, ComPtr<ID3D11UnorderedAccessView>& uav);
		static void CreateBuffer(ID3D11Device* device, const UINT elementSize, const void* initData, DXGI_FORMAT format, ComPtr<ID3D11Buffer>& buffer, ComPtr<ID3D11ShaderResourceView>& srv);
	};
}