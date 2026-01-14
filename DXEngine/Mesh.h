#pragma once
#include "pch.h"
#include "Texture2D.h"	

namespace DE {
	struct Mesh {
		ComPtr<ID3D11Buffer> vertexBuffer;
		ComPtr<ID3D11Buffer> indexBuffer;

		ComPtr<ID3D11Buffer> basicMaterialConstGPU;
		ComPtr<ID3D11Buffer> materialConstGPU;

		Texture2D albedoTexture;
		Texture2D diffuseTexture;
		Texture2D specularTexture;

		Texture2D emissiveTexture;
		Texture2D heightTexture;
		Texture2D normalTexture;
		Texture2D aoTexture;
		Texture2D metallicRoughnessTexture;

		UINT indexCount = 0;
		UINT vertexCount = 0; // Normal Vector ·»´õ¸µ¿ë
		UINT stride = 0;
		UINT offset = 0;
	};
}