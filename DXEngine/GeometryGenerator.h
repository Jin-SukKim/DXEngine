#pragma once
#include "MeshData.h"

namespace DE{

class GeometryGenerator
{
public:
	static MeshData MakeTriangle(const float scale = 1.f, const Vector2 texScale = Vector2(1.f));
};

}
