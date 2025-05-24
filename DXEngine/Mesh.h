#pragma once
#include "pch.h"
#include "Texture2D.h"	

namespace DE {
	struct Mesh {
		ComPtr<ID3D11Buffer> vertexBuffer;
		ComPtr<ID3D11Buffer> indexBuffer;

		ComPtr<ID3D11Buffer> meshConstGPU;
		ComPtr<ID3D11Buffer> basicMaterialConstGPU;

		Texture2D albedoTexture;
		Texture2D diffuseTexture;
		Texture2D specularTexture;

		Texture2D emissiveTexture;
		Texture2D normalTexture;
		Texture2D aoTexture;
		Texture2D metallicroughnessTexture;

		UINT indexCount = 0;
	};
}