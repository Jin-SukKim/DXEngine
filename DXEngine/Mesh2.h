#pragma once
#include "pch.h"
#include "Texture2D.h"	

namespace DE {
	struct Mesh2 {
		ComPtr<ID3D11Buffer> vertexBuffer;
		ComPtr<ID3D11Buffer> indexBuffer;
		std::vector<Vertex> vertexCPU;
		std::vector<Vector3> vertexPosCPU;
		std::vector<uint32_t> indexCPU;

		UINT indexCount = 0;
		UINT vertexCount = 0;
		UINT stride = 0;
		UINT offset = 0;
	};
}