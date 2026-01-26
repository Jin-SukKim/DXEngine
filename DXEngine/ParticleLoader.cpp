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

	// bakedPath 확인
	if (jsonData.contains("Spawn") && jsonData["Spawn"].contains("bakedPath")) {
		std::string bakedPath = jsonData["Spawn"]["bakedPath"];
		target->LoadBakedSpawnData(bakedPath);
	}

	// 모듈 로드
	for (auto& [key, value] : jsonData.items()) {
		if (key == "Name" || key == "Duration" || key == "CompletionDelay" || key == "SubEmitters")
			continue;

		auto module = ParticleModuleFactory::Create(key);
		if (module) {
			module->LoadFromJson(value);
			target->AddModule(std::move(module));
		}
	}
	
	// Emitter Duration 설정
	if (jsonData.contains("Duration")) {
		target->SetDuration(jsonData["Duration"]);
	}
	
	if (jsonData.contains("CompletionDelay")) {
		target->SetCompletionDelay(jsonData["CompletionDelay"]);
	}
	
	// Sub-Emitter 설정
	if (jsonData.contains("SubEmitters") && jsonData["SubEmitters"].is_array()) {
		for (const auto& subJson : jsonData["SubEmitters"]) {
			SubEmitterEntry entry;
			
			if (subJson.contains("path")) {
				std::string path = subJson["path"];
				entry.emitterPath = std::wstring(path.begin(), path.end());
			}
			
			if (subJson.contains("trigger")) {
				std::string trigger = subJson["trigger"];
				if (trigger == "OnStart") entry.trigger = EmitterEvent::OnStart;
				else if (trigger == "OnDurationEnd") entry.trigger = EmitterEvent::OnDurationEnd;
				else if (trigger == "OnComplete") entry.trigger = EmitterEvent::OnComplete;
			}
			
			if (subJson.contains("inheritPosition")) {
				entry.inheritPosition = subJson["inheritPosition"];
			}
			
			target->AddSubEmitter(entry);
		}
	}
}

template<>
void ParticleLoader::ApplyJsonTo<ParticleSystem>(ParticleSystem* target, const json& jsonData) {
	if (!target) return;

	target->LoadFromJson(jsonData);
}

}
