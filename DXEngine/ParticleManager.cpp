#include "pch.h"
#include "ParticleManager.h"
#include "ParticleLoader.h"
#include "RenderBase.h"

namespace DE {
	void ParticleManager::Update(const float& dt)
	{
		// 활성 시스템만 업데이트
		for (auto* system : m_activeSystems) {
			if (system) {
				system->Update(dt);
			}
		}
	}

	void ParticleManager::Render()
	{
		if (m_activeSystems.empty()) return;

		// 파티클 렌더링을 위한 PSO 설정은 RenderModule에서 처리
		// 여기서는 순회만 진행
		for (auto* system : m_activeSystems) {
			if (system) {
				system->Render();
			}
		}

		// 렌더링 후 BasicPSO로 복원
		GET_SINGLE(RenderBase)->SetPipelineState(RenderBase::graphicsCommon.basic.solidPSO);
	}

	void ParticleManager::RegisterActiveSystem(ParticleSystem* system)
	{
		if (!system) return;
		
		// 중복 체크
		auto it = std::find(m_activeSystems.begin(), m_activeSystems.end(), system);
		if (it == m_activeSystems.end()) {
			m_activeSystems.push_back(system);
		}
	}

	void ParticleManager::UnregisterActiveSystem(ParticleSystem* system)
	{
		if (!system) return;
		
		auto it = std::find(m_activeSystems.begin(), m_activeSystems.end(), system);
		if (it != m_activeSystems.end()) {
			m_activeSystems.erase(it);
		}
	}

	ParticleSystem* ParticleManager::CreateSystem(const std::wstring& path)
	{
		// 프로토타입 찾기
		auto prototypeIt = m_prototypes.find(path);
		ParticleSystem* prototype = nullptr;

		if (prototypeIt != m_prototypes.end()) {
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
			m_prototypes[path] = std::move(newSystem);
		}

		// 복제본 생성
		auto cloned = std::make_unique<ParticleSystem>(*prototype);

		// 복제본 초기화 (독립 버퍼 생성)
		cloned->Initialize();
		cloned->OnSpawn();

		auto* clonedPtr = cloned.get();
		m_instances.push_back(std::move(cloned));

		// 자동으로 활성 시스템에 등록
		RegisterActiveSystem(clonedPtr);

		printf("[ParticleManager] CreateInstance: %p (Cloned)\n", clonedPtr);

		return clonedPtr;
	}
}