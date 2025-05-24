#pragma once
//#include "Texture2D.h"

namespace DE {
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
		static void CreateConstantBuffer(ComPtr<ID3D11Device>& device, const T_CONSTANT& constantData, ComPtr<ID3D11Buffer>& constantBuffer) {
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
		static void CreateTexture(ComPtr<ID3D11Device>& device, const std::string& filename, Texture2D& texture);
	};
}