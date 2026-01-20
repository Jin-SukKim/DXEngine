#pragma once
#include "pch.h"

namespace DE {

	struct Material{
		std::string name;

		int albedoTexture = -1;
		int emissiveTexture = -1;
		int heightTexture = -1;
		int normalTexture = -1;
		int aoTexture = -1;
		int metallicTexture = -1;
		int roughnessTexture = -1;
	};
}