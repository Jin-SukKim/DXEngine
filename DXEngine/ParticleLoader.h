#pragma once
#include "ParticleEmitter.h"

namespace DE {

class ParticleLoader
{
public:
	static std::unique_ptr<ParticleEmitter> Load(const std::wstring& filePath);
	static std::wstring presetPath;
};
}

