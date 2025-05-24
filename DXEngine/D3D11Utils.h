#pragma once
//#include "Texture2D.h"

namespace DE {
	class Texture2D;
	inline void ThrowIfFailed(HRESULT hr) {
		if (FAILED(hr)) {
			// ������� �� ���⿡ breakpoint�� ����
			throw std::exception();
		}
	}
	class D3D11Utils
	{
	public:
		template<typename T_VERTEX>
		static void CreateVertexBuffer(ComPtr<ID3D11Device>& device, const std::vector<T_VERTEX>& vertices, ComPtr<ID3D11Buffer>& vertexBuffer) {
			// Buffer�� ��� ���� ����
			D3D11_BUFFER_DESC desc = {};
			ZeroMemory(&desc, sizeof(desc));
			desc.Usage = D3D11_USAGE_DEFAULT; // GPU read/write
			desc.ByteWidth = UINT(sizeof(T_VERTEX) * vertices.size()); // �迭 ��ü ũ��
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			desc.CPUAccessFlags = 0; // 0 if no CPU access if necessary
			desc.StructureByteStride = sizeof(T_VERTEX); // ������ �ϳ��� �������� ũ��

			// CPU���� GPU�� �����͸� ������ � �����͸� � �������� ������ ����
			D3D11_SUBRESOURCE_DATA bufferData = { 0 };
			bufferData.pSysMem = vertices.data(); // �����͸� ��𼭺��� ������ ��������
			bufferData.SysMemPitch = 0;
			bufferData.SysMemSlicePitch = 0;

			// GPU���� �޸� ����
			ThrowIfFailed(device->CreateBuffer(&desc, &bufferData, vertexBuffer.GetAddressOf()));
		}

		static void CreateIndexBuffer(ComPtr<ID3D11Device>& device, const std::vector<uint32_t>& indices, ComPtr<ID3D11Buffer>& indexBuffer);
		
		// ConstantBuffer�� ���� Update���� ���� �� ������ �ٲ��ֹǷ� CPU���� ����, GPU���� �бⰡ ������ Buffer�� ����
		template<typename T_CONSTANT>
		static void CreateConstantBuffer(ComPtr<ID3D11Device>& device, const T_CONSTANT& constantData, ComPtr<ID3D11Buffer>& constantBuffer) {
			D3D11_BUFFER_DESC desc = {};
			ZeroMemory(&desc, sizeof(desc));
			desc.Usage = D3D11_USAGE_DYNAMIC; // CPU���� ����, GPU���� �б� ����
			desc.ByteWidth = UINT(sizeof(constantData));
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // CPU���� ���� ����
			desc.StructureByteStride = 0; // �迭�� �ƴϹǷ� 0

			D3D11_SUBRESOURCE_DATA bufferData = { 0 };
			bufferData.pSysMem = &constantData;
			bufferData.SysMemPitch = 0;
			bufferData.SysMemSlicePitch = 0;

			ThrowIfFailed(device->CreateBuffer(&desc, &bufferData, constantBuffer.GetAddressOf()));
		}

		// Vertex Shader�� InputLayout ����
		static void CreateVSAndIL(ComPtr<ID3D11Device>& device, const std::wstring& filename, const std::vector<D3D11_INPUT_ELEMENT_DESC>& inputElements, ComPtr<ID3D11VertexShader>& vertexShader, ComPtr<ID3D11InputLayout>& inputLayout);
		// Pixel Shader ����
		static void CreatePS(ComPtr<ID3D11Device>& device, const std::wstring& filename, ComPtr<ID3D11PixelShader>& pixelShader);

		// Usage�� Dynamic���� ������ Buffer�� GPU���� GPU �޸𸮷� ������ ����
		template<typename T_DATA>
		static void UpdateBuffer(ComPtr<ID3D11DeviceContext>& context, const T_DATA& bufferData, ComPtr<ID3D11Buffer>& buffer) {
			D3D11_MAPPED_SUBRESOURCE ms;
			// GPU �޸𸮿� ����
			context->Map(buffer.Get(), NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
			// CPU �����͸� GPU �޸𸮿� ����
			memcpy(ms.pData, &bufferData, sizeof(bufferData));
			// GPU �޸� ���� ����
			context->Unmap(buffer.Get(), NULL);
			
		}

		// Texture2D ����
		static void CreateTexture(ComPtr<ID3D11Device>& device, const std::string& filename, Texture2D& texture);
	};
}