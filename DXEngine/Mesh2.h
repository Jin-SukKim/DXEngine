#pragma once
#include "pch.h"
#include "Texture2D.h"	

namespace DE {
	struct Mesh2 {
		ComPtr<ID3D11Buffer> vertexBuffer;
		ComPtr<ID3D11Buffer> indexBuffer;

		UINT indexCount = 0;
		UINT vertexCount = 0;
		UINT stride = 0;
		UINT offset = 0;

		int materialIdx;
	};
}