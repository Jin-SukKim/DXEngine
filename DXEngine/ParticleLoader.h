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

	// ParticleEmitter는 ParticleSystem이 Initialize를 호출하므로
	// 여기서는 Hot-Reload 콜백만 등록하고 Initialize 호출 제거
	T* rawPtr = instance.get();
	auto callback = [rawPtr, fullPath]() {
		std::ifstream reloadFile(fullPath);
		if (reloadFile.is_open()) {
			json newJson;
			reloadFile >> newJson;

			ApplyJsonTo(rawPtr, newJson);
		}

		// ParticleEmitter의 경우: OwnerSystem이 있으면 System 전체 재초기화
		// ParticleSystem의 경우: 자체 Initialize 호출
		if constexpr (std::is_same_v<T, ParticleEmitter>) {
			// Emitter는 개별 초기화 불가 - 로그만 출력하거나 무시
			// 실제로는 ParticleSystem의 Hot-Reload에서 처리해야 함
		}
		else {
			rawPtr->Initialize();
			rawPtr->OnSpawn();
		}
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
	file.close();

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

		if constexpr (std::is_same_v<T, ParticleEmitter>) {
			// Emitter는 개별 초기화 불가
		}
		else {
			target->Initialize();
			target->OnSpawn();
		}
	};

	auto id = FileWatcher::Get().Register(fullPath, callback);
	target->SetHotReloadInfo(fullPath, id);
}

template<>
void ParticleLoader::ApplyJsonTo<ParticleEmitter>(ParticleEmitter* target, const json& jsonData);

template<>
void ParticleLoader::ApplyJsonTo<ParticleSystem>(ParticleSystem* target, const json& jsonData);


}

