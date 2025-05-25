#pragma once
#include "pch.h"

#include "Texture2D.h"

namespace DE {
	struct MeshData {
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		std::string albedoTextureFilename;
		//std::string diffuseTextureFilename;
		//std::string specularTextureFilename;
		std::string emissiveTextureFilename;
		std::string normalTextureFilename;
		std::string heightTextureFilename;
		std::string aoTextureFilename;
		std::string metallicTextureFilename;
		std::string roughnessTextureFilename;
		std::string opacityTextureFilename;
	};
}