#pragma once
#include "pch.h"

namespace DE {
	struct Vertex {
		Vector3 position;
		Vector3 normalModel;
		Vector2 texcoord;
		Vector3 tangentModel;
		// Vector3 biTangentModell; // biTangent는 Shader에서 계산
	};
};