#include "pch.h"
#include "ParticleLoader.h"
#include "ParticleModuleFactory.h"
#include "FileWatcher.h"
#include "SpawnModule.h"

namespace DE {
std::wstring ParticleLoader::presetPath = L"..\\Assets\\";

template<>
void ParticleLoader::ApplyJsonTo<ParticleEmitter>(ParticleEmitter* target, const json& jsonData) {
	if (!target) return;

	target->ClearModules();

	// Spawn 모듈의 bakedPath 확인 (ParticleEmitter에 로드하기 위해)
	if (jsonData.contains("Spawn") && jsonData["Spawn"].contains("bakedPath")) {
		std::string bakedPath = jsonData["Spawn"]["bakedPath"];
		target->LoadBakedSpawnData(bakedPath);
	}

	for (auto& [key, value] : jsonData.items()) {
		if (key == "name")
			continue;

		auto module = ParticleModuleFactory::Create(key);
		if (module) {
			module->LoadFromJson(value);
			target->AddModule(std::move(module));
		}
	}
}

template<>
void ParticleLoader::ApplyJsonTo<ParticleSystem>(ParticleSystem* target, const json& jsonData) {
	if (!target) return;

	target->LoadFromJson(jsonData);
}

}
