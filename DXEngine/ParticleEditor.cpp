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
		//m_spawner->SetActorType<Firework>();
		//m_spawner->SetSpawnMode(SpawnMode::Interval);
		//m_spawner->SetSpawnInterval(0.1f);
		//m_spawner->SetSpawnBox(Vector3(5.0f, 0.5f, 1.f));
		//m_spawner->SetMaxActiveParticles(100);

		//m_spawner2 = AddObject<ParticleSpawner>(L"FireworkSpawner");
		//m_spawner2->SetScene(this);
		//m_spawner2->SetActorType<IceEffect>();
		//m_spawner2->SetSpawnMode(SpawnMode::Interval);
		//m_spawner2->SetSpawnInterval(0.01f);
		//m_spawner2->SetSpawnBox(Vector3(15.0f, 5.0f, 1.f));
		//m_spawner2->SetMaxActiveParticles(3000);

		//m_rose = AddObject<RoseEffect>(L"RoseOrbit");

		// [�ó����� ����] 1�� Ÿ��: ���� ����Ʈ (HolySword)
		//m_test1 = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\Combination\\HolySword\\System_HolySword.json");
		m_test1 = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\Explosion\\Explosion.json");

		// m_test2�� �ó����� �߰�(15��)�� �����ϱ� ���� �����
		m_test2 = nullptr;
		m_test3 = nullptr;

		//for (int i = 0; i < 1000; ++i) {
		//	m_fireTests.push_back(AddObject<FireEffect>(L"FireEffect" + i));
		//}

		//m_smoke = AddObject<SmokeActor>(L"SmokeEffect");

		//m_metalSpark = SpawnEffect<EffectActor>(L"MetalSpark", L"Particles\\MetalSparkEffect.json", Vector3(0.f, 0.f, 3.f));
		m_sparkBurst = SpawnEffect<EffectActor>(L"SparkBurst", L"Particles\\SparkBurstEffect.json", Vector3(3.f, 0.f, 0.f));
		//m_ember = SpawnEffect<EffectActor>(L"Ember", L"Particles\\EmberEffect.json", Vector3(-3.f, 0.f, 0.f));
		m_curlFirefly = SpawnEffect<EffectActor>(L"CurlFirefly", L"Particles\\Effects\\ForceModule\\CurlNoise_Firefly.json", Vector3(6.f, 0.f, -3.f));
		m_swordClash = SpawnEffect<EffectActor>(L"SwordClash", L"Particles\\Effects\\Combination\\SwordClash\\System_SwordClash.json", Vector3(-3.f, 1.f, 0.f));
		//m_curlMagicAura = SpawnEffect<EffectActor>(L"CurlMagicAura", L"Particles\\Effects\\ForceModule\\CurlNoise_MagicAura.json", Vector3(9.f, 0.f, -3.f));
		//m_curlSpiritWisp = SpawnEffect<EffectActor>(L"CurlSpiritWisp", L"Particles\\Effects\\ForceModule\\CurlNoise_SpiritWisp.json", Vector3(12.f, 0.f, -3.f));

		//m_sample = AddObject<SampleActor>(L"SampleActor");

		m_test = SpawnEffect<BoxMeshEffect>(L"MoxMeshEffect", L"Particles\\Effects\\BoxMesh\\BoxMesh.json", Vector3(0.f, 0.f, 03.f));
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

		float spacing = 2.0f; // ���� (�ʿ信 ���� ����)
		int width = 32;       // �� �ٿ� ��ġ�� ���� (���� ����)

		//for (int i = 0; i < 1000; ++i) {
		//	auto tr = m_fireTests[i]->GetComponent<TransformComponent>();
		//	if (tr) {
		//		// x�� ������ �������� 0 ~ 31 �ݺ�
		//		int x = i % width;

		//		// z�� ������ �������� 32������ 1�� ���� (�ٹٲ�)
		//		int z = i / width;

		//		// Vector3(x��ǥ, ����, z��ǥ) * ����
		//		tr->SetPos(Vector3(x * spacing, 0.0f, z * spacing));
		//	}
		//}

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
		//ClickEffectManager::Get().TriggerPreset("Firework");
		// �� �����ӿ� 100���� ���� �õ�! (���İ��� ��õ ���� ��)
		for (int i = 0; i < 100; ++i)
		{
			// 1. ����Ʈ ���� (Smoke�� Firework ��)
			Vector3 randomPos(
				(rand() % 100 - 50) * 1.0f,
				(rand() % 20) * 1.0f,
				(rand() % 100 - 50) * 1.0f
			);

			// IceEffect �Ǵ� HolySwordEffect�� �����ϰ� ����
			EffectActor* effect = nullptr;
			//if (rand() % 2 == 0) {
				//effect = SpawnEffect<IceEffect>(L"ClickIce", L"", randomPos);
				effect = SpawnEffect<SmokeEffect>(L"ClickSmoke", L"", randomPos);
			//}
			//else {
			//	effect = SpawnEffect<HolySwordEffect>(L"ClickHolySword", L"", randomPos);
			//}

			if (effect) {
			}
			else {
				// 2. [�Ѱ� ����] ���� ���� (Nullptr ��ȯ��)
				// �޸� Ǯ(Particle/Emitter/System Slot) �� �ϳ��� �� ��
				break;
			}
		}
	}
	void ParticleEditor::ClickDestroy()
	{
	}
}