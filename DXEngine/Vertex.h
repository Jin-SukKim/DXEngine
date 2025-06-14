#pragma once
#include "pch.h"

namespace DE {
	struct Vertex {
		Vector3 position;
		Vector3 normalModel;
		Vector2 texcoord;
		Vector3 tangentModel; // Normal Vector를 위해 World 좌표계로 변환시킬때 필요한 값
		// Vector3 biTangentModel; // biTangent는 Shader에서 계산
	};
};