#include "pch.h"
#include "ParticleLoader.h"
#include "ParticleModuleFactory.h"

namespace DE {

std::unique_ptr<ParticleEmitter> ParticleLoader::Load(const std::wstring& filePath)
{
	std::ifstream file(filePath);
	if (!file.is_open())
		return nullptr;

	json j;
	file >> j;

	std::wstring name = L"Particle";
	if (j.contains("name")) {
		std::string n = j["name"];
		name = std::wstring(n.begin(), n.end());
	}

	auto emitter = std::make_unique<ParticleEmitter>(name);

	for (auto& [key, value] : j.items()) {
		if (key == "name")
			continue;

		auto module = ParticleModuleFactory::Create(key);
		if (module) {
			module->LoadFromJson(value);
			emitter->AddModule(std::move(module));
		}
	}

	return std::move(emitter);
}
}
