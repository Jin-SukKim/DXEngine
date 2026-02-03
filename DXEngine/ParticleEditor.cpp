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

#include <imgui.h>

namespace DE {
	// 테스트용 이펙트 목록
	static const std::vector<const char*> s_effectPaths = {
		"Particles\\Firework.json",
		"Particles\\TestEffect.json",
		"Particles\\SmokeEffect.json",
		"Particles\\Effects\\Combination\\HolySword\\System_HolySword.json",
		// 필요시 추가
	};

	ParticleEditor::ParticleEditor() : Scene(), m_click(m_lButton)
	{
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

		m_spanwer = AddObject<ParticleSpawner>(L"FireworkSpawner");
		m_spanwer->SetScene(this);
		m_spanwer->SetActorType<Firework>();
		m_spanwer->SetSpawnMode(SpawnMode::Interval);
		m_spanwer->SetSpawnInterval(0.5f);
		m_spanwer->SetSpawnBox(Vector3(5.0f, 0.5f, 1.f));
		m_spanwer->SetMaxActiveParticles(12);
	}

	ParticleEditor::~ParticleEditor()
	{
		for (auto* sys : m_testSystems) {
			if (sys) ParticleManager::Get().DestroyInstance(sys);
		}
		m_testSystems.clear();

		if (m_test1) {
			ParticleManager::Get().DestroyInstance(m_test1);
			m_test1 = nullptr;
		}
		if (m_test2) {
			ParticleManager::Get().DestroyInstance(m_test2);
			m_test2 = nullptr;
		}
	}

	void ParticleEditor::Initialize()
	{
		Scene::Initialize();
	}

	void ParticleEditor::Update(const float& dt)
	{
		Scene::Update(dt);
		FileWatcher::Get().Update();

		if (m_runTest) {
			RunMemoryPoolTest(dt);
		}
	}

	void ParticleEditor::UpdateGUI()
	{
		Scene::UpdateGUI();
		ParticleManager::Get().RenderDebugGUI();

		if (ImGui::Begin("Memory Pool Test")) {
			ImGui::Text("Test Systems: %zu", m_testSystems.size());
			ImGui::Text("Test Phase: %d", m_testPhase);

			ImGui::Separator();

			// 이펙트 선택
			static int selectedEffect = 0;
			static char customPath[256] = "";

			if (ImGui::BeginCombo("Effect Preset", s_effectPaths[selectedEffect])) {
				for (int i = 0; i < static_cast<int>(s_effectPaths.size()); ++i) {
					if (ImGui::Selectable(s_effectPaths[i], selectedEffect == i)) {
						selectedEffect = i;
					}
				}
				ImGui::EndCombo();
			}

			ImGui::InputText("Custom Path", customPath, sizeof(customPath));
			ImGui::SameLine();
			if (ImGui::Button("Use Custom")) {
				// Custom path 사용시 생성
				if (strlen(customPath) > 0) {
					std::wstring wpath(customPath, customPath + strlen(customPath));
					if (auto* sys = ParticleManager::Get().CreateSystem(wpath)) {
						m_testSystems.push_back(sys);
					}
				}
			}

			// 선택된 이펙트로 생성
			auto GetSelectedPath = [&]() -> std::wstring {
				const char* path = s_effectPaths[selectedEffect];
				return std::wstring(path, path + strlen(path));
				};

			if (ImGui::Button("Create")) {
				if (auto* sys = ParticleManager::Get().CreateSystem(GetSelectedPath())) {
					m_testSystems.push_back(sys);
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Destroy Last")) {
				if (!m_testSystems.empty()) {
					ParticleManager::Get().DestroyInstance(m_testSystems.back());
					m_testSystems.pop_back();
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Destroy All")) {
				for (auto* sys : m_testSystems) {
					ParticleManager::Get().DestroyInstance(sys);
				}
				m_testSystems.clear();
			}

			ImGui::Separator();

			// Burst 테스트
			static int burstCount = 5;
			static bool randomEffect = false;
			ImGui::SliderInt("Burst Count", &burstCount, 1, 20);
			ImGui::Checkbox("Random Effect", &randomEffect);

			if (ImGui::Button("Burst Create")) {
				for (int i = 0; i < burstCount; ++i) {
					int idx = randomEffect ? (rand() % s_effectPaths.size()) : selectedEffect;
					const char* path = s_effectPaths[idx];
					std::wstring wpath(path, path + strlen(path));
					if (auto* sys = ParticleManager::Get().CreateSystem(wpath)) {
						m_testSystems.push_back(sys);
					}
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Burst Destroy")) {
				for (int i = 0; i < burstCount && !m_testSystems.empty(); ++i) {
					ParticleManager::Get().DestroyInstance(m_testSystems.back());
					m_testSystems.pop_back();
				}
			}

			ImGui::Separator();

			// 자동 테스트
			static bool useRandomInAutoTest = true;
			ImGui::Checkbox("Use Random in Auto Test", &useRandomInAutoTest);
			m_useRandomEffect = useRandomInAutoTest;

			if (!m_runTest) {
				if (ImGui::Button("Start Auto Test")) {
					m_runTest = true;
					m_testPhase = 0;
					m_testTimer = 0.0f;
				}
			}
			else {
				if (ImGui::Button("Stop Auto Test")) {
					m_runTest = false;
				}
			}

			ImGui::TextWrapped("Auto Test: Create/Destroy systems to test allocation/deallocation");
		}
		ImGui::End();
	}

	void ParticleEditor::Render()
	{
		Scene::Render();
	}

	void ParticleEditor::ClickEvent()
	{
		ClickEffectManager::Get().TriggerPreset("Firework");
	}

	void ParticleEditor::RunMemoryPoolTest(const float& dt)
	{
		m_testTimer += dt;

		const float interval = 0.2f;
		if (m_testTimer < interval) return;
		m_testTimer = 0.0f;

		auto CreateRandomSystem = [this]() -> ParticleSystem* {
			int idx = m_useRandomEffect ? (rand() % s_effectPaths.size()) : 0;
			const char* path = s_effectPaths[idx];
			std::wstring wpath(path, path + strlen(path));
			return ParticleManager::Get().CreateSystem(wpath);
			};

		switch (m_testPhase) {
		case 0: // Phase 0: 연속 생성
			if (m_testSystems.size() < 20) {
				if (auto* sys = CreateRandomSystem()) {
					m_testSystems.push_back(sys);
				}
			}
			else {
				m_testPhase = 1;
			}
			break;

		case 1: // Phase 1: 랜덤 삭제 (홀수 인덱스)
			for (int i = static_cast<int>(m_testSystems.size()) - 1; i >= 0; i -= 2) {
				ParticleManager::Get().DestroyInstance(m_testSystems[i]);
				m_testSystems.erase(m_testSystems.begin() + i);
			}
			m_testPhase = 2;
			break;

		case 2: // Phase 2: 빈 공간에 다시 할당 (단편화 테스트)
			if (m_testSystems.size() < 15) {
				if (auto* sys = CreateRandomSystem()) {
					m_testSystems.push_back(sys);
				}
			}
			else {
				m_testPhase = 3;
			}
			break;

		case 3: // Phase 3: 전부 삭제
			if (!m_testSystems.empty()) {
				ParticleManager::Get().DestroyInstance(m_testSystems.back());
				m_testSystems.pop_back();
			}
			else {
				m_testPhase = 0;
			}
			break;
		}
	}
}