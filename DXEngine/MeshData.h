#pragma once
#include "pch.h"

namespace DE {
	struct MeshData {
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
	};
}