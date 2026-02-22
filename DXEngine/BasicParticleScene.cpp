#include "pch.h"
#include "BasicParticleScene.h"
#include "ParticleModuleFactory.h"
#include "SpawnModule.h"
#include "VisualModule.h"
#include "ForceModule.h"
#include "VortexModule.h"
#include "RenderModule.h"	
#include "MaterialModule.h"	
#include "OrbitModule.h"
#include "ClickEffectManager.h"
#include "ParticleSpawner.h"
#include "Firework.h"
#include "SquareActor.h"
#include "SpawnEffectsActor.h"
#include "BillboardEffectsActor.h"
#include "ForceEffectsActor.h"
#include "MaterialEffectsActor.h"
#include "MeshEffectsActor.h"
#include "OrbitEffectsActor.h"
#include "SubEmitterEffectsActor.h"
#include "VisualEffectsActor.h"
#include "VortexEffectsActor.h"
#include "MagicEffect.h"
#include "SnowActor.h"
#include "PortalEffect.h"
#include "BreathEffect.h"
#include "HolySwordEffect.h"
#include "IceEffect.h"
#include "NecroEffect.h"
#include "NightSkyEffect.h"
#include "PhoenixEffect.h"
#include "StarEffect.h"
#include "ThunderEffect.h"
#include "TsunamiEffect.h"
#include "AppBase.h"
#include "InputManager.h"

namespace DE {
	BasicParticleScene::BasicParticleScene() : Scene(), m_click(m_lButton)
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

		m_spawner.reserve(9);

		ParticleSpawner* spawner = AddObject<ParticleSpawner>(L"FireworkSpawner");
		spawner->SetScene(this);
		spawner->SetActorType<Firework>();
		spawner->SetSpawnMode(SpawnMode::Interval); // or SpawnMode::Continuous
		spawner->SetSpawnInterval(0.2f);
		spawner->SetSpawnBox(Vector3(30.0f, 0.5f, 1.f));
		spawner->SetMaxActiveParticles(100);

		m_spawner.push_back(spawner);
		for (int i = 0; i < 3; ++i) {
			spawner = AddObject<ParticleSpawner>(L"MagicEffect");
			spawner->SetScene(this);
			switch (i) {
				case 0: spawner->SetActorType<MagicEffect>(); break;
				case 1: spawner->SetActorType<BreathEffect>(); break;
				case 2: spawner->SetActorType<HolySwordEffect>(); break;
				case 3: spawner->SetActorType<IceEffect>(); break;
				case 4: spawner->SetActorType<NecroEffect>(); break;
				case 5: spawner->SetActorType<PhoenixEffect>(); break;
				case 6: spawner->SetActorType<StarEffect>(); break;
				case 7: spawner->SetActorType<TsunamiEffect>(); break;
			}
			spawner->SetSpawnMode(SpawnMode::Interval);
			spawner->SetSpawnInterval(0.5f);
			spawner->SetSpawnBox(Vector3(1.f, 1.f, 1.f));
			spawner->SetMaxActiveParticles(1);

			m_spawner.push_back(spawner);
		}

		//m_magic = AddObject<MagicEffect>(L"MagicEffect");
		//m_breathEffect = AddObject<BreathEffect>(L"BreathEffect");
		//m_holySwordEffect = AddObject<HolySwordEffect>(L"HolySwordEffect");
		//m_iceEffect = AddObject<IceEffect>(L"IceEffect");
		//m_necroEffect = AddObject<NecroEffect>(L"NecroEffect");
		//m_phoenixEffect = AddObject<PhoenixEffect>(L"PhoenixEffect");
		//m_starEffect = AddObject<StarEffect>(L"StarEffect");
		//m_tsunamiEffect = AddObject<TsunamiEffect>(L"TsunamiEffect");

		m_spawnModule = AddObject<SpawnEffectsActor>(L"SpawnEffectsActor");
		m_billboardModule = AddObject<BillboardEffectsActor>(L"BillboardEffectsActor");
		m_forceModule = AddObject<ForceEffectsActor>(L"ForceEffectsActor");
		//m_materialModule = AddObject<MaterialEffectsActor>(L"MaterialEffectsActor");
		m_meshModule = AddObject<MeshEffectsActor>(L"MeshEffectsActor");
		m_orbitModule = AddObject<OrbitEffectsActor>(L"OrbitEffectsActor");
		m_subEmitSystem = AddObject<SubEmitterEffectsActor>(L"SubEmitterEffectsActor");
		m_visualModule = AddObject<VisualEffectsActor>(L"VisualEffectsActor");
		m_vortexModule = AddObject<VortexEffectsActor>(L"VortexEffectsActor");
		m_portal = AddObject<PortalEffect>(L"PortalEffect");
		m_snow = AddObject<SnowActor>(L"SnowActor");
		m_nightSky = AddObject<NightSkyEffect>(L"NightSkyEffect");
	}

	BasicParticleScene::~BasicParticleScene()
	{
	}

	void BasicParticleScene::Initialize()
	{
		Scene::Initialize();

		auto* tr = m_spawner[0]->GetComponent<TransformComponent>();
		if (tr) {
			Vector3 pos = tr->GetPos();
			pos.z += 30;
			tr->SetPos(pos);
		}

		for (int i = 1; i < 3; ++i) {
			tr = m_spawner[i]->GetComponent<TransformComponent>();
			if (tr) {
				Vector3 pos = tr->GetPos();

				// 4x2 ��ġ (4��, 2��)
				int row = i / 4;  // �� �ε��� (0 �Ǵ� 1)
				int col = i % 4;  // �� �ε��� (0, 1, 2, 3)

				pos.x += col * 5.f;  // X�� �������� 10 ����
				pos.z += row * 5.f;  // Z�� �������� 10 ����

				tr->SetPos(pos);
			}
		}

		m_snow->SetPosOffset(Vector3(-17.f, 3.f, 0.f));
		m_spawnModule->SetPosOffset(Vector3(-15.f, 0.f, -10.f));
		m_billboardModule->SetPosOffset(Vector3(-15.f, 0.f, 0.f));
		m_forceModule->SetPosOffset(Vector3(-15.f, 0.f, 10.f));
		//m_materialModule->SetPosOffset(Vector3(3.f, 0.f, 0.f));
		m_orbitModule->SetPosOffset(Vector3(0.f, 0.f, 5.f));
		m_subEmitSystem->SetPosOffset(Vector3(0.f, 0.f, 10.f));
		m_visualModule->SetPosOffset(Vector3(0.f, 0.f, 15.f));
		m_vortexModule->SetPosOffset(Vector3(0.f, 0.f, -10.f));
		m_portal->SetPosOffset(Vector3(10.f, 0.f, 5.f));
		m_nightSky->SetPosOffset(Vector3(0.f, -10.f, 20.f));

		//m_magic->SetPosOffset(Vector3(10.f, 0.f, 0.f));
		//m_breathEffect->SetPosOffset(Vector3(10.f, 0.f, 10.f));
		//m_holySwordEffect->SetPosOffset(Vector3(15.f, 0.f, 0.f));
		//m_iceEffect->SetPosOffset(Vector3(15.f, 0.f, 5.f));
		//m_necroEffect->SetPosOffset(Vector3(15.f, 0.f, 10.f));
		//m_phoenixEffect->SetPosOffset(Vector3(20.f, 0.f, 0.f));
		//m_starEffect->SetPosOffset(Vector3(20.f, 0.f, 5.f));
		//m_tsunamiEffect->SetPosOffset(Vector3(20.f, 0.f, 10.f));

		AppBase::GetInputManager().BindInputAction(m_lButton, InputState::Pressed, this, &BasicParticleScene::ClickEvent);
	}

	void BasicParticleScene::Update(const float& dt)
	{
		Scene::Update(dt);
	}

	void BasicParticleScene::ClickEvent()
	{
		ClickEffectManager::Get().TriggerPreset("Thunder");
	}

	void BasicParticleScene::UpdateGUI()
	{
		Scene::UpdateGUI();
	}
}