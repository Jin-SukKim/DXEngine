#pragma once
#include "pch.h"

namespace DE {
	struct Mesh {
		ComPtr<ID3D11Buffer> vertexBuffer;
		ComPtr<ID3D11Buffer> indexBuffer;

		ComPtr<ID3D11Buffer> meshConstGPU;

		UINT indexCount = 0;
	};
}