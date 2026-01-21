#pragma once
#include "ParticleEmitter.h"
#include "ParticleSystem.h"
namespace DE {

class ParticleLoader
{
public:
	template<typename T>
	static std::unique_ptr<T> Load(const std::wstring& filePath);
	
	template<typename T>
	static void Load(const std::wstring& filePath, T* target);
private:
	template<typename T>
	static void ApplyJsonTo(T* target, const json& jsonData) {}

public:
	static std::wstring presetPath;
};

template<typename T>
std::unique_ptr<T> ParticleLoader::Load(const std::wstring& filePath) {
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

	auto instance = std::make_unique<T>(name);

	ApplyJsonTo(instance.get(), j);

	T* rawPtr = instance.get();
	auto callback = [rawPtr, fullPath]() {
		std::ifstream reloadFile(fullPath);
		if (reloadFile.is_open()) {
			json newJson;
			reloadFile >> newJson;

			ApplyJsonTo(rawPtr, newJson);
		}

		rawPtr->Initialize();
		rawPtr->OnSpawn();
	};

	auto id = FileWatcher::Get().Register(fullPath, callback);
	instance->SetHotReloadInfo(fullPath, id);

	return std::move(instance);
}

template<typename T>
inline void ParticleLoader::Load(const std::wstring& filePath, T* target)
{
	std::wstring fullPath = presetPath + filePath;
	std::ifstream file(fullPath);
	if (!file.is_open())
		return;

	json j;
	file >> j;

	std::wstring name = L"Particle";
	if (j.contains("Name")) {
		std::string n = j["Name"];
		name = std::wstring(n.begin(), n.end());
		target->SetName(name);
	}

	ApplyJsonTo(target, j);

	auto callback = [target, fullPath]() {
		std::ifstream reloadFile(fullPath);
		if (reloadFile.is_open()) {
			json newJson;
			reloadFile >> newJson;

			ApplyJsonTo(target, newJson);
		}

		target->Initialize();
		target->OnSpawn();
	};

	auto id = FileWatcher::Get().Register(fullPath, callback);
	target->SetHotReloadInfo(fullPath, id);
}

template<>
void ParticleLoader::ApplyJsonTo<ParticleEmitter>(ParticleEmitter* target, const json& jsonData);

template<>
void ParticleLoader::ApplyJsonTo<ParticleSystem>(ParticleSystem* target, const json& jsonData);


}

