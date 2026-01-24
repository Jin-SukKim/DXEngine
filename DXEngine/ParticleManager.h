#pragma once
#include "ParticleSystem.h"

namespace DE {

struct ParticlePreset {
	std::unique_ptr<ParticleSystem> prototype; // 원본
	std::wstring filePath;
	FileWatcher::CallbackID watcherID = 0;
};

class ParticleManager
{
public:
	static ParticleManager& Get() {
		static ParticleManager instance;
		return instance;
	}

	void Update(const float& dt);
	void Render();
	
	// 활성 파티클 시스템 등록/해제
	void RegisterActiveSystem(ParticleSystem* system);
	void UnregisterActiveSystem(ParticleSystem* system);

	ParticleSystem* CreateSystem(const std::wstring& path);

private:
	// 프로토타입 저장소 (파일 경로별 원본)
	std::unordered_map<std::wstring, std::unique_ptr<ParticleSystem>> m_prototypes;
	
	// 생성된 모든 인스턴스 (소유권 관리)
	std::vector<std::unique_ptr<ParticleSystem>> m_instances;
	
	// 렌더링할 활성 시스템들 (포인터만 저장, 소유권 없음)
	std::vector<ParticleSystem*> m_activeSystems;
};

}

