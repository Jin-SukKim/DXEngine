#pragma once
#include "pch.h"

namespace DE {
	__declspec(align(256)) struct MeshConstants {
		Matrix world;
	};

	__declspec(align(256)) struct GlobalConstants {
		Matrix view;
		Matrix proj;
	};
}