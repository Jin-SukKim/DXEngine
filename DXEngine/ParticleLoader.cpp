#include "pch.h"
#include "ParticleLoader.h"
#include "ParticleModuleFactory.h"
#include "FileWatcher.h"

namespace DE {
std::wstring ParticleLoader::presetPath = L"..\\Assets\\";

template<>
void ParticleLoader::ApplyJsonTo<ParticleEmitter>(ParticleEmitter* target, const json& jsonData) {
	if (!target) return;

	target->ClearModules();

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
