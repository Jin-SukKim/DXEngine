#include "pch.h"
#include "D3D11Utils.h"
#include "Image.h"
#include "Texture2D.h"

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
		UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
		compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		ComPtr<ID3DBlob> errorBlob;

		ThrowIfFailed(D3DCompileFromFile(filename.c_str(), 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", compileFlags, 0, &shaderBlob, &errorBlob));

		device->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, &pixelShader);
	}

	void D3D11Utils::CreateTexture(ComPtr<ID3D11Device>& device, const std::string& filename, Texture2D& texture)
	{
		Image img(L"Image");
		if (!img.Load(filename))
			throw std::exception();

		// Texture ����
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = img.GetWidth();
		desc.Height = img.GetHeight();
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // �Ϲ����� �̹��� ������ ������ uint8_t�̱⿡ R8G8B8A8_UNORM ���
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_IMMUTABLE; 
		// Shader Resource View�� ���
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0; // No CPU Access

		// � �����ͷ� �ʱ�ȭ���� ����
		D3D11_SUBRESOURCE_DATA initData;
		initData.pSysMem = img.GetImage().data();
		initData.SysMemPitch = desc.Width * sizeof(uint8_t) * img.GetChannels();
		initData.SysMemSlicePitch = 0; // �����Ͱ� �迭�� ��� ���

		ThrowIfFailed(device->CreateTexture2D(&desc, &initData, texture.GetAddressOfTexture()));
		ThrowIfFailed(device->CreateShaderResourceView(texture.GetTexture(), nullptr, texture.GetAddressOfSRV()));
	}
}