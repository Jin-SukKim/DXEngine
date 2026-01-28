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
	target->ClearSubEmitters();

	// Spawn 모듈의 bakedPath 확인 (ParticleEmitter에 로드하기 위해)
	if (jsonData.contains("Spawn") && jsonData["Spawn"].contains("bakedPath")) {
		std::string bakedPath = jsonData["Spawn"]["bakedPath"];
		target->LoadBakedSpawnData(bakedPath);
	}

	for (auto& [key, value] : jsonData.items()) {
		// Module에서 설정이 아니라 Emitter의 설정값으로 밑에서 따로 설정
		if (key == "Name" || key == "Duration" || key == "CompletionDelay" || key == "SubEmitters")
			continue;

		auto module = ParticleModuleFactory::Create(key);
		if (module) {
			module->LoadFromJson(value);
			target->AddModule(std::move(module));
		}
	}

	// Emitter 설정
	if (jsonData.contains("Duration")) 
		target->SetDuration(jsonData["Duration"]);
	if (jsonData.contains("CompletionDelay")) 
		target->SetCompletionDelay(jsonData["CompletionDelay"]);

	// SubEmitter 설정
	if (jsonData.contains("SubEmitters") && jsonData["SubEmitters"].is_array()) {
		for (const auto& subJson : jsonData["SubEmitters"]) {
			SubEmitter sub;

			if (subJson.contains("path")) {
				std::string path = subJson["path"];
				sub.emitterPath = std::wstring(path.begin(), path.end());
			}

			if (subJson.contains("trigger")) {
				std::string trigger = subJson["trigger"];
				if (trigger == "OnStart") sub.trigger = EmitterEvent::OnStart;
				else if (trigger == "OnDurationEnd") sub.trigger = EmitterEvent::OnDurationEnd;
				else if (trigger == "OnComplete") sub.trigger = EmitterEvent::OnComplete;
			}

			if (subJson.contains("inheritPosition")) 
				sub.inheritPosition = subJson["inheritPosition"];

			target->AddSubEmitter(sub);
		}
	}
}

template<>
void ParticleLoader::ApplyJsonTo<ParticleSystem>(ParticleSystem* target, const json& jsonData) {
	if (!target) return;

	target->LoadFromJson(jsonData);
}

}
