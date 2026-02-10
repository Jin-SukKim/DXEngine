#include "pch.h"
#include "ParticleEditor.h"
#include "ParticleEmitter.h"
#include "SquareActor.h"
#include "ParticleModuleFactory.h"
#include "SpawnModule.h"
#include "VisualModule.h"
#include "ForceModule.h"
#include "RenderModule.h"
#include "ParticleLoader.h"
#include "FileWatcher.h"
#include "SampleActor.h"
#include "TransformComponent.h"
#include "GeometryGenerator.h"
#include "ModelManager.h"

#include "ModelComponent.h"
#include "TextureSpawnBake.h"
#include "EffectActor.h"
#include "ParticleSpawner.h"
#include "ClickEffectManager.h"
#include "AppBase.h"
#include "InputManager.h"

#include "ParticleModuleFactory.h"

#include "SpawnModule.h"
#include "VisualModule.h"
#include "ForceModule.h"
#include "VortexModule.h"
#include "RenderModule.h"	
#include "MaterialModule.h"	
#include "Firework.h"
#include "OrbitModule.h"
#include "RoseEffect.h"
#include "ParticleManager.h"
#include "TestActor.h"

namespace DE {
	ParticleEditor::ParticleEditor() : Scene(), m_Lclick(m_lButton), m_Rclick(m_rButton)
	{
		// ... (기존 모듈 등록 코드는 유지) ...
		ParticleModuleFactory::Register<SpawnModule>("Spawn");
		ParticleModuleFactory::Register<VisualModule>("Visual");
		ParticleModuleFactory::Register<ForceModule>("Force");
		ParticleModuleFactory::Register<VortexModule>("Vortex");
		ParticleModuleFactory::Register<OrbitModule>("Orbit");
		ParticleModuleFactory::Register<BillboardRenderModule>("BillboardRender");
		ParticleModuleFactory::Register<MaterialModule>("Material");
		ParticleModuleFactory::Register<MeshRenderModule>("MeshRender");

		ClickEffectManager::Get().Initialize();
		ClickEffectManager::Get().SetScene(this);
		ground = AddObject<SquareActor>(L"Ground");

		// 기존 Spawner는 잠시 꺼두거나 유지해도 됨 (여기선 유지)
		m_spanwer = AddObject<ParticleSpawner>(L"FireworkSpawner");
		m_spanwer->SetScene(this);
		m_spanwer->SetActorType<Firework>();
		m_spanwer->SetSpawnMode(SpawnMode::Interval);
		m_spanwer->SetSpawnInterval(0.05f);
		m_spanwer->SetSpawnBox(Vector3(5.0f, 0.5f, 1.f));
		m_spanwer->SetMaxActiveParticles(500);

		//m_rose = AddObject<RoseEffect>(L"RoseOrbit");

		// [시나리오 시작] 1번 타자: 지속 이펙트 (HolySword)
		m_test1 = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\Combination\\HolySword\\System_HolySword.json");
		//m_test1 = ParticleManager::Get().CreateSystem(L"Particles\\FireEffect.json");

		// m_test2는 시나리오 중간(15초)에 생성하기 위해 비워둠
		m_test2 = nullptr;
		m_test3 = nullptr;
	}

	ParticleEditor::~ParticleEditor()
	{
		// 소멸 시 깔끔하게 정리
		if (m_test1) {
			ParticleManager::Get().DestroyInstance(m_test1);
			m_test1 = nullptr;
		}
		if (m_test2) {
			ParticleManager::Get().DestroyInstance(m_test2);
			m_test2 = nullptr;
		}
		if (m_test3) {
			ParticleManager::Get().DestroyInstance(m_test3);
			m_test3 = nullptr;
		}
		for (auto* sys : m_stressSystems) {
			ParticleManager::Get().DestroyInstance(sys);
		}
		m_stressSystems.clear();
	}

	void ParticleEditor::Initialize()
	{
		Scene::Initialize();

		AppBase::GetInputManager().BindInputAction(m_lButton, InputState::Pressed, this, &ParticleEditor::ClickEvent);
		AppBase::GetInputManager().BindInputAction(m_rButton, InputState::Pressed, this, &ParticleEditor::ClickDestroy);
	}

	void ParticleEditor::Update(const float& dt)
	{
		Scene::Update(dt);
		FileWatcher::Get().Update();

		//StressTest(dt);
	}

	void ParticleEditor::StressTest(const float& dt)
	{
		// =========================================================
		// [Dynamic Stress Test Scenario]
		// =========================================================
		m_stressTime += dt;

		static float burstTimer = 0.0f;
		static float spawnTimer = 0.0f;
		static float randomDeleteTimer = 0.0f; // [추가] 랜덤 삭제용 타이머

											   // 1. [0~5초] 평온한 상태 (HolySword만 재생됨)

											   // 2. [5~10초] 순간적인 이펙트 (Firework) 0.5초마다 폭발
		if (m_stressTime >= 5.0f && m_stressTime < 10.0f) {
			burstTimer += dt;
			if (burstTimer >= 0.5f) {
				burstTimer = 0.0f;
				auto sys = ParticleManager::Get().CreateSystem(L"Particles\\Firework.json");
				if (sys) m_stressSystems.push_back(sys);
			}
		}

		// 3. [10~15초] 계속 스폰되는 이펙트 (Smoke) 0.1초마다 대량 생성
		if (m_stressTime >= 10.0f && m_stressTime < 15.0f) {
			spawnTimer += dt;
			if (spawnTimer >= 0.1f) {
				spawnTimer = 0.0f;
				auto sys = ParticleManager::Get().CreateSystem(L"Particles\\SmokeEffect.json");
				if (sys) m_stressSystems.push_back(sys);
			}
		}

		// [신규] 4. [5~25초 전구간] 랜덤하게 선택된 Effect 다수 삭제 (Cluster Deletion)
		// 0.8초마다 "3개 ~ 8개" 정도의 이펙트를 한꺼번에 제거하여 큰 구멍을 만듭니다.
		if (m_stressTime >= 5.0f && m_stressTime < 25.0f) {
			randomDeleteTimer += dt;
			if (randomDeleteTimer >= 0.8f) {
				randomDeleteTimer = 0.0f;

				// 한 번에 삭제할 개수 결정 (예: 3개 ~ 8개 사이 랜덤)
				int deleteCount = (rand() % 400) + 100;

				for (int i = 0; i < deleteCount; ++i)
				{
					// 더 이상 지울 게 없으면 중단 (안전 장치)
					if (m_stressSystems.empty()) break;

					// 랜덤 인덱스 선택
					int idx = rand() % m_stressSystems.size();
					ParticleSystem* victim = m_stressSystems[idx];

					// 강제 파괴 (메모리 반환)
					if (victim) {
						ParticleManager::Get().DestroyInstance(victim);
					}

					// 관리 리스트에서 제거 (Swap & Pop으로 빠르게 제거)
					if (idx != m_stressSystems.size() - 1) {
						m_stressSystems[idx] = m_stressSystems.back();
					}
					m_stressSystems.pop_back();
				}
			}
		}

		// 5. [15초] 중간에 갑자기 추가되는 이펙트 (DragonBreath)
		if (m_stressTime >= 15.0f && m_test2 == nullptr) {
			m_test2 = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\Combination\\DragonBreath\\System_DragonBreath.json");
		}

		if (m_stressTime >= 17.5f && m_test3 == nullptr) {
			m_test3 = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\Combination\\Ice\\System_IceExplosion.json");
		}

		// 6. [20초] 갑자기 사라지는 이펙트 (HolySword 제거)
		if (m_stressTime >= 20.0f && m_test1 != nullptr) {
			ParticleManager::Get().DestroyInstance(m_test1);
			m_test1 = nullptr;
		}

		// 7. [25초] 강제 삭제 및 초기화 (Loop)
		if (m_stressTime >= 25.0f) {
			// 남아있는 동적 시스템 싹 정리
			for (auto* sys : m_stressSystems) {
				ParticleManager::Get().DestroyInstance(sys);
			}
			m_stressSystems.clear();

			if (m_test2) {
				ParticleManager::Get().DestroyInstance(m_test2);
				m_test2 = nullptr;
			}
			if (m_test3) {
				ParticleManager::Get().DestroyInstance(m_test3);
				m_test3 = nullptr;
			}

			// 초기 상태 복구 (HolySword 재생성)
			if (m_test1 == nullptr) {
				m_test1 = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\Combination\\HolySword\\System_HolySword.json");
			}

			// 타이머 리셋
			m_stressTime = 0.0f;
			burstTimer = 0.0f;
			spawnTimer = 0.0f;
			randomDeleteTimer = 0.0f;

			// 25초 주기 완료 시 로그 출력
			OutputDebugStringA(("=== Cycle Complete ===\n"
				"Rebuild Count: " + std::to_string(ParticleManager::Get().GetRebuildCount()) + "\n"
				"Avg Rebuild Time: " + std::to_string(ParticleManager::Get().GetAvgRebuildTime()) + " ms\n"
				).c_str());
		}
	}

	void ParticleEditor::UpdateGUI()
	{
		Scene::UpdateGUI();
		ParticleManager::Get().RenderMemoryPoolGUI();
	}

	void ParticleEditor::Render()
	{
		Scene::Render();
	}

	void ParticleEditor::ClickEvent()
	{
		//ClickEffectManager::Get().TriggerPreset("Firework");
		// 한 프레임에 100개씩 생성 시도! (순식간에 수천 개가 됨)
		for (int i = 0; i < 100; ++i)
		{
			// 1. 이펙트 생성 (Smoke나 Firework 등)
			//auto sys = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\Combination\\Ice\\System_IceExplosion.json");
			auto sys = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\Combination\\HolySword\\System_HolySword.json");

			if (sys) {
				// 성공 시 리스트에 추가 (나중에 정리용)
				m_stressSystems.push_back(sys);
			}
			else {
				// 2. [한계 도달] 생성 실패 (Nullptr 반환됨)
				// 메모리 풀(Particle/Emitter/System Slot) 중 하나가 꽉 참
				static bool oomLogged = false;
				if (!oomLogged) {
					OutputDebugStringA(">>> [LIMIT REACHED] Memory Pool or System Slots FULL! <<<\n");
					oomLogged = true;
				}
				break; // 더 이상 생성 불가
			}
		}
	}
	void ParticleEditor::ClickDestroy()
	{
		for (auto* sys : m_stressSystems) {
			ParticleManager::Get().DestroyInstance(sys);
		}
		m_stressSystems.clear();
		OutputDebugStringA(">>> [RESET] All Stress Systems Destroyed. <<<\n");
	}
}