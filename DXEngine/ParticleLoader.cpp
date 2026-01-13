#include "pch.h"
#include "ParticleLoader.h"
#include "ParticleModuleFactory.h"

namespace DE {
	std::wstring ParticleLoader::presetPath = L"..\\Assets\\Particles\\";

std::unique_ptr<ParticleEmitter> ParticleLoader::Load(const std::wstring& filePath)
{
	std::wstring fullPath = presetPath + filePath;
	std::ifstream file(fullPath);
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

	ApplyJsonToEmitter(emitter.get(), j);

	// Hot-Reload µî·Ï
	ParticleEmitter* rawPtr = emitter.get();
	std::wstring absPath = std::filesystem::absolute(fullPath);

	auto callback = [rawPtr, fullPath]() {
		std::ifstream reloadFile(fullPath);

		if (reloadFile.is_open()) {
			json newJson;
			reloadFile >> newJson;
			ApplyJsonToEmitter(rawPtr, newJson);
			std::wcout << L"Reloaded: " << fullPath << std::endl;
		}

		rawPtr->Initialize();
	};

	auto id = FileWatcher::Get().Register(fullPath, callback);
	emitter->SetHotReloadInfo(fullPath, id);

	return std::move(emitter);
}
void ParticleLoader::ApplyJsonToEmitter(ParticleEmitter* emitter, const json& jsonData)
{
	if (!emitter) return;

	emitter->ClearModules();

	for (auto& [key, value] : jsonData.items()) {
		if (key == "name")
			continue;

		auto module = ParticleModuleFactory::Create(key);
		if (module) {
			module->LoadFromJson(value);
			emitter->AddModule(std::move(module));
		}
	}
}
}
