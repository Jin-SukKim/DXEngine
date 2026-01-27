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
#include "SnowActor.h"

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

		m_spanwer = AddObject<ParticleSpawner>(L"FireworkSpawner");
		m_spanwer->SetScene(this);
		m_spanwer->SetActorType<Firework>();
		m_spanwer->SetSpawnMode(SpawnMode::Interval); // or SpawnMode::Continuous
		m_spanwer->SetSpawnInterval(0.5f);
		m_spanwer->SetSpawnBox(Vector3(5.0f, 0.5f, 1.f));
		m_spanwer->SetMaxActiveParticles(20);


		m_spawnModule = AddObject<SpawnEffectsActor>(L"SpawnEffectsActor");
		m_snow = AddObject<SnowActor>(L"SnowActor");
	}

	BasicParticleScene::~BasicParticleScene()
	{
	}

	void BasicParticleScene::Initialize()
	{
		Scene::Initialize();
	}

	void BasicParticleScene::Update(const float& dt)
	{
		Scene::Update(dt);
	}

	void BasicParticleScene::ClickEvent()
	{
		ClickEffectManager::Get().TriggerPreset("Burst");
	}
}