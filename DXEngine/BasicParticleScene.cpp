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

		//m_spanwer = AddObject<ParticleSpawner>(L"FireworkSpawner");
		//m_spanwer->SetScene(this);
		//m_spanwer->SetActorType<Firework>();
		//m_spanwer->SetSpawnMode(SpawnMode::Interval); // or SpawnMode::Continuous
		//m_spanwer->SetSpawnInterval(0.5f);
		//m_spanwer->SetSpawnBox(Vector3(30.0f, 0.5f, 1.f));
		//m_spanwer->SetMaxActiveParticles(100);


		m_spawnModule = AddObject<SpawnEffectsActor>(L"SpawnEffectsActor");
		m_billboardModule = AddObject<BillboardEffectsActor>(L"BillboardEffectsActor");
		m_forceModule = AddObject<ForceEffectsActor>(L"ForceEffectsActor");
		m_materialModule = AddObject<MaterialEffectsActor>(L"MaterialEffectsActor");
		m_meshModule = AddObject<MeshEffectsActor>(L"MeshEffectsActor");
		m_orbitModule = AddObject<OrbitEffectsActor>(L"OrbitEffectsActor");
		m_subEmitSystem = AddObject<SubEmitterEffectsActor>(L"SubEmitterEffectsActor");
		m_visualModule = AddObject<VisualEffectsActor>(L"VisualEffectsActor");
		m_vortexModule = AddObject<VortexEffectsActor>(L"VortexEffectsActor");
		m_magic = AddObject<MagicEffect>(L"MagicEffect");
		m_portal = AddObject<PortalEffect>(L"PortalEffect");
		m_snow = AddObject<SnowActor>(L"SnowActor");
		m_breathEffect = AddObject<BreathEffect>(L"BreathEffect");
		m_holySwordEffect = AddObject<HolySwordEffect>(L"HolySwordEffect");
		m_iceEffect = AddObject<IceEffect>(L"IceEffect");
		m_necroEffect = AddObject<NecroEffect>(L"NecroEffect");
		m_nightSky = AddObject<NightSkyEffect>(L"NightSkyEffect");
		m_phoenixEffect = AddObject<PhoenixEffect>(L"PhoenixEffect");
		m_starEffect = AddObject<StarEffect>(L"StarEffect");
		m_thunderEffect = AddObject<ThunderEffect>(L"ThunderEffect");
		m_tsunamiEffect = AddObject<TsunamiEffect>(L"TsunamiEffect");
	}

	BasicParticleScene::~BasicParticleScene()
	{
	}

	void BasicParticleScene::Initialize()
	{
		Scene::Initialize();

		//auto* tr = m_spanwer->GetComponent<TransformComponent>();
		//if (tr) {
		//	Vector3 pos = tr->GetPos();
		//	pos.z += 30;
		//	tr->SetPos(pos);
		//}

		m_snow->SetPosOffset(Vector3(-17.f, 3.f, 0.f));
		m_spawnModule->SetPosOffset(Vector3(-15.f, 0.f, -10.f));
		m_billboardModule->SetPosOffset(Vector3(-15.f, 0.f, 0.f));
		m_forceModule->SetPosOffset(Vector3(-15.f, 0.f, 10.f));
		m_materialModule->SetPosOffset(Vector3(3.f, 0.f, 0.f));
		m_orbitModule->SetPosOffset(Vector3(0.f, 0.f, 5.f));
		m_subEmitSystem->SetPosOffset(Vector3(0.f, 0.f, 10.f));
		m_visualModule->SetPosOffset(Vector3(0.f, 0.f, 15.f));
		m_vortexModule->SetPosOffset(Vector3(0.f, 0.f, -10.f));
		m_magic->SetPosOffset(Vector3(10.f, 0.f, 0.f));
		m_portal->SetPosOffset(Vector3(10.f, 0.f, 5.f));
		m_breathEffect->SetPosOffset(Vector3(10.f, 0.f, 10.f));
		m_holySwordEffect->SetPosOffset(Vector3(15.f, 0.f, 0.f));
		m_iceEffect->SetPosOffset(Vector3(15.f, 0.f, 5.f));
		m_iceEffect->SetPosOffset(Vector3(15.f, 0.f, 10.f));
		m_phoenixEffect->SetPosOffset(Vector3(20.f, 0.f, 0.f));
		m_starEffect->SetPosOffset(Vector3(20.f, 0.f, 5.f));
		m_thunderEffect->SetPosOffset(Vector3(20.f, 0.f, 10.f));
		m_tsunamiEffect->SetPosOffset(Vector3(20.f, 0.f, 15.f));
		m_nightSky->SetPosOffset(Vector3(0.f, -10.f, 20.f));

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
}