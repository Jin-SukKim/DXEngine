#include "pch.h"
#include "ParticleManager.h"
#include "ParticleLoader.h"

namespace DE {
	void ParticleManager::Update(const float& dt)
	{
		for (const auto& [path, system] : m_particles) {
			system->Update(dt);
		}
	}
	void ParticleManager::Render()
	{
		for (const auto& [path, system] : m_particles) {
			system->Render();
		}
	}

	ParticleSystem* ParticleManager::CreateSystem(const std::wstring& path)
	{
		// 원본 찾기
		auto prototypeIt = m_particles.find(path);
		ParticleSystem* prototype = nullptr;

		if (prototypeIt != m_particles.end()) {
			// 캐시 히트
			prototype = prototypeIt->second.get();
			printf("[ParticleManager] Cache HIT: %ls\n", path.c_str());
		}
		else {
			// 캐시 미스 - 원본 로드
			printf("[ParticleManager] Cache MISS: Loading %ls\n", path.c_str());

			auto newSystem = ParticleLoader::Load<ParticleSystem>(path);
			if (!newSystem) {
				printf("[ParticleManager] Failed to load: %ls\n", path.c_str());
				return nullptr;
			}

			// 원본 초기화
			newSystem->Initialize();
			newSystem->OnSpawn();

			prototype = newSystem.get();
			m_particles[path] = std::move(newSystem);
		}

		// 복제본 생성
		auto cloned = std::make_unique<ParticleSystem>(*prototype);

		// 복제본 초기화 (독립 버퍼 생성)
		cloned->Initialize();
		cloned->OnSpawn();

		// 임시: 복제본을 path + "_instance_N" 키로 저장
		static int instanceCounter = 0;
		std::wstring instanceKey = path + L"_instance_" + std::to_wstring(instanceCounter++);

		auto* clonedPtr = cloned.get();
		m_particles[instanceKey] = std::move(cloned);

		printf("[ParticleManager] CreateInstance: %ls (Cloned)\n", instanceKey.c_str());

		return clonedPtr;
	}
}