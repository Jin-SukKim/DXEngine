#pragma once
#include "ParticleEmitter.h"

namespace DE {

class ParticleLoader
{
public:
	static std::unique_ptr<ParticleEmitter> Load(const std::wstring& filePath);
	
private:
	// JSON data를 Emitter에 적용하는 로직
	static void ApplyJsonToEmitter(ParticleEmitter* emitter, const json& jsonData);

public:
	static std::wstring presetPath;


};
}

