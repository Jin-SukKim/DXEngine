#pragma once
#include "pch.h"

namespace DE {
	__declspec(align(256)) struct MeshConstants {
		Matrix world;
		Matrix view;
		Matrix proj;
	};
}