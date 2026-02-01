#pragma once
#include "AppendBuffer.h"
#include "ComputeShader.h"
#include "Mesh.h"
#include "MeshData.h"

struct BakeConsts {
	UINT indexCount = 0;
	float threshold = 0.f;
	Vector2 padding;
	Vector4 channelMask = Vector4(1.f, 1.f, 1.f, 0.f);
};

namespace DE {
class TextureSpawnBake
{
public:
	static TextureSpawnBake Get() {
		static TextureSpawnBake instance;
		return instance;
	}

	void Bake(const std::string& name, const std::string& basePath, bool isGLTF, const std::string& materialName, const std::string& textureType, BakeConsts& consts, const std::string& outputPath);
	void LoadBakedData(const std::string& path, std::vector<Vector3>& outBuffer, UINT& outCount);
private:
	void initBuffers(ID3D11Device* device, ID3D11DeviceContext* context, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, BakeConsts& consts);
	void setTextureSRV(const int& materialIdx, const std::string& textureType);
	void initShader(ID3D11Device* device);
	void dispatch(ID3D11DeviceContext* context);
	void downloadResult(ID3D11Device* device, ID3D11DeviceContext* context);
	void saveToBin(ID3D11DeviceContext* context, const std::string& outputPath);

private:
	StructuredBuffer<Vertex> m_meshVertex;
	StructuredBuffer<uint32_t> m_meshIndices;
	ConstantBuffer<BakeConsts> m_consts;

	AppendBuffer<Vector3> m_outputBuffer; // Spawn Position만 저장할 Buffer
	ComputeShader m_bakeCS;
	UINT m_maxPoints = 1000000;
	std::string m_presetPath = "..\\Assets\\";
	ComPtr<ID3D11ShaderResourceView> m_srv;
	UINT m_validCount = 0;
};
}
