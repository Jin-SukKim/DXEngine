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
#include "SmokeActor.h"
#include "FireEffect.h"
#include "IceEffect.h"
#include "HolySwordEffect.h"
#include "SmokeEffect.h"
#include "BoxMeshEffect.h"
#include "ExplosionEffect.h"
#include "SwordClashEffect.h"

namespace DE {
	ParticleEditor::ParticleEditor() : Scene(), m_Lclick(m_lButton), m_Rclick(m_rButton)
	{
		// ... (���� ��� ��� �ڵ�� ����) ...
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
		// ���� Spawner�� ��� ���ΰų� �����ص� �� (���⼱ ����)
		//m_spawner = AddObject<ParticleSpawner>(L"FireworkSpawner");
		//m_spawner->SetScene(this);
		//m_spawner->SetActorType<SwordClashEffect>();
		//m_spawner->SetSpawnMode(SpawnMode::Interval);
		//m_spawner->SetSpawnInterval(0.01f);
		//m_spawner->SetSpawnBox(Vector3(15.0f, 5.0f, 1.f));
		//m_spawner->SetMaxActiveParticles(1000);

		//m_spawner2 = AddObject<ParticleSpawner>(L"FireworkSpawner");
		//m_spawner2->SetScene(this);
		//m_spawner2->SetActorType<ExplosionEffect>();
		//m_spawner2->SetSpawnMode(SpawnMode::Interval);
		//m_spawner2->SetSpawnInterval(0.01f);
		//m_spawner2->SetSpawnBox(Vector3(15.0f, 5.0f, 1.f));
		//m_spawner2->SetMaxActiveParticles(1000);

		//m_rose = AddObject<RoseEffect>(L"RoseOrbit");

		//m_test1 = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\Explosion\\Explosion.json");
		//m_test1 = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\Custom\\Custom.json");
		
		// ── Realistic ────────────────────────────────────────────────────
		m_rain = SpawnEffect<EffectActor>(
			L"Rain",
			L"Particles\\Effects\\Realistic\\Rain\\System_Rain.json",
			Vector3(0.f, 0.f, 0.f));

		// ── Spectacular ──────────────────────────────────────────────────
		//m_arcaneCircle = SpawnEffect<EffectActor>(
		//	L"ArcaneCircle",
		//	L"Particles\\Effects\\Spectacular\\ArcaneCircle\\System_ArcaneCircle.json",
		//	Vector3(0.f, 0.f, 0.f));

		m_crystalShatter = SpawnEffect<EffectActor>(
			L"CrystalShatter",
			L"Particles\\Effects\\Spectacular\\CrystalShatter\\System_CrystalShatter.json",
			Vector3(0.f, 0.f, 0.f));

		m_galaxySwirl = SpawnEffect<EffectActor>(
			L"GalaxySwirl",
			L"Particles\\Effects\\Spectacular\\GalaxySwirl\\System_Galaxy.json",
			Vector3(0.f, 0.f, 0.f));

		// ── UnrealQuality ────────────────────────────────────────────────
		m_portalGateway = SpawnEffect<EffectActor>(
			L"PortalGateway",
			L"Particles\\Effects\\UnrealQuality\\PortalGateway\\System_PortalGateway.json",
			Vector3(0.f, 0.f, 0.f));

		// ── Explosion ────────────────────────────────────────────────────
		m_explosion = SpawnEffect<EffectActor>(
			L"Explosion",
			L"Particles\\Effects\\Explosion\\Explosion.json",
			Vector3(0.f, 0.f, 0.f));

		// ── Combination ──────────────────────────────────────────────────
		m_holySword = SpawnEffect<EffectActor>(
			L"HolySword",
			L"Particles\\Effects\\Combination\\HolySword\\System_HolySword.json",
			Vector3(0.f, 0.f, 0.f));

		m_swordClash = SpawnEffect<EffectActor>(
			L"SwordClash",
			L"Particles\\Effects\\Combination\\SwordClash\\System_SwordClash.json",
			Vector3(0.f, 0.f, 0.f));

		// ── Magic ────────────────────────────────────────────────────────
		m_magicCast = SpawnEffect<EffectActor>(
			L"MagicCast",
			L"Particles\\Effects\\Magic\\System_MagicCast.json",
			Vector3(0.f, 0.f, 0.f));

		// ── ForceModule ──────────────────────────────────────────────────
		m_curlNoiseFirefly = SpawnEffect<EffectActor>(
			L"CurlNoiseFirefly",
			L"Particles\\Effects\\ForceModule\\CurlNoise_Firefly.json",
			Vector3(0.f, 0.f, 0.f));

		// ── Misc / Custom ────────────────────────────────────────────────
		m_fireEffect = SpawnEffect<EffectActor>(
			L"FireEffect",
			L"Particles\\FireEffect.json",
			Vector3(0.f, 0.f, 0.f));

		m_sparkBurst = SpawnEffect<EffectActor>(
			L"SparkBurst",
			L"Particles\\SparkBurstEffect.json",
			Vector3(0.f, 0.f, 0.f));

		m_fog = SpawnEffect<EffectActor>(
			L"Fog",
			L"Particles\\Effects\\Custom\\Fog.json",
			Vector3(0.f, 0.f, 0.f));

		m_custom = SpawnEffect<EffectActor>(
			L"Custom",
			L"Particles\\Effects\\Custom\\Custom.json",
			Vector3(0.f, 0.f, 0.f));

		m_test2 = nullptr;
		m_test3 = nullptr;

		//m_sample = AddObject<SampleActor>(L"SampleActor");

	}

	ParticleEditor::~ParticleEditor()
	{
		// �Ҹ� �� ����ϰ� ����
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
	}

	void ParticleEditor::Initialize()
	{
		Scene::Initialize();

		// ── Effect 배치 위치 설정 ──────────────────────────────────────────
		// 각 Effect를 일정 간격으로 배열하여 씬에서 한눈에 확인할 수 있도록 배치합니다.
		// 필요에 따라 위치·간격을 조정하세요.

		struct EffectPlacement { EffectActor** actor; Vector3 pos; };
		const EffectPlacement placements[] =
		{
			// ── Row 0 (z = 0) : Realistic / Spectacular ──────────────────
			{ &m_rain,             Vector3(-15.f, 0.f,  0.f) },
			//{ &m_arcaneCircle,     Vector3(-7.f, 0.f,  0.f) },
			{ &m_crystalShatter,   Vector3(0.f, 0.f,  0.f) },
			{ &m_galaxySwirl,      Vector3(7.f, 0.f,  0.f) },
			{ &m_portalGateway,    Vector3(15.f, 0.f,  0.f) },

			// ── Row 1 (z = 10) : Explosion / Combination ─────────────────
			{ &m_explosion,        Vector3(-10.f, 0.f, 10.f) },
			{ &m_holySword,        Vector3(0.f, 0.f, 10.f) },
			{ &m_swordClash,       Vector3(10.f, 0.f, 10.f) },

			// ── Row 2 (z = 20) : Magic / Force / Custom ──────────────────
			{ &m_magicCast,        Vector3(-14.f, 0.f, 20.f) },
			{ &m_curlNoiseFirefly, Vector3(-7.f, 0.f, 20.f) },
			{ &m_fireEffect,       Vector3(0.f, 0.f, 20.f) },
			{ &m_sparkBurst,       Vector3(7.f, 0.f, 20.f) },
			{ &m_fog,              Vector3(14.f, 0.f, 20.f) },
			{ &m_custom,           Vector3(21.f, 0.f, 20.f) },
		};

		for (const auto& p : placements)
		{
			if (*p.actor)
			{
				auto* tr = (*p.actor)->GetComponent<TransformComponent>();
				if (tr) tr->SetPos(p.pos);
			}
		}

		//auto tr = m_spawner2->GetComponent<TransformComponent>();
		//if (tr)
		//	tr->SetPos(Vector3(25.f, 0.f, 0.f));
		//m_smoke->SetPosOffset(Vector3(3.f, -2.5f, 0.f));
		//AppBase::GetInputManager().BindInputAction(m_lButton, InputState::Pressed, this, &ParticleEditor::ClickEvent);
		//AppBase::GetInputManager().BindInputAction(m_rButton, InputState::Pressed, this, &ParticleEditor::ClickDestroy);
	}

	void ParticleEditor::Update(const float& dt)
	{
		Scene::Update(dt);
		FileWatcher::Get().Update();
	}

	void ParticleEditor::UpdateGUI()
	{
		Scene::UpdateGUI();
	}

	void ParticleEditor::Render()
	{
		Scene::Render();
	}

	void ParticleEditor::ClickEvent()
	{
		for (int i = 0; i < 100; ++i)
		{
			Vector3 randomPos(
				(rand() % 100 - 50) * 1.0f,
				(rand() % 20) * 1.0f,
				(rand() % 100 - 50) * 1.0f);

			EffectActor* effect = SpawnEffect<IceEffect>(L"ClickIce", L"", randomPos);
			if (!effect) break; // 슬롯 소진 시 중단
		}
	}

	void ParticleEditor::ClickDestroy()
	{
	}
}