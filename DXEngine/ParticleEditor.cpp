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

// https://dev.epicgames.com/documentation/en-us/unreal-engine/particle-system-user-guide?application_version=4.27
namespace DE {
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

		// ClickEffectManager에 Scene 설정
		ClickEffectManager::Get().Initialize();
		ClickEffectManager::Get().SetActiveScene(this);

		ground = AddObject<SquareActor>(L"Ground");
		
		m_sample = AddObject<SampleActor>(L"Sample");

		m_spanwer = AddObject<ParticleSpawner>(L"TempSpawner");
		m_spanwer->SetOwnerScene(this);  // ParticleSpawner에 Scene 설정
		m_spanwer->SetActorType<Firework>();
		//m_spanwer->SetParticlePreset(L"Particles\\TempEffect.json");
		m_spanwer->SetSpawnMode(SpawnMode::Interval);
		m_spanwer->SetSpawnInterval(0.5f);
		m_spanwer->SetSpawnRadius(2.0f);
		m_spanwer->SetMaxActiveParticles(20);

		//m_firework = AddObject<Firework>(L"Firework");
		m_rose = AddObject<RoseEffect>(L"RoseOrbit");
	}

	ParticleEditor::~ParticleEditor()
	{
	}

	void ParticleEditor::Initialize()
	{	
		//BakeConsts consts = {};
		//consts.threshold = 0.1f;
		//consts.channelMask = Vector4(0.299f, 0.587f, 0.114f, 0.f);
		//TextureSpawnBake::Get().Bake(
		//	"DamagedHelmet.gltf", "DamagedHelmet\\", true,
		//	"Materials\\DamagedHelmet.json",
		//	"emissive", consts, "Models\\DamagedHelmet\\emissive.bin");
		//exit(0);
		Scene::Initialize();

		TransformComponent* tr;
		
		tr = m_sample->GetComponent<TransformComponent>();
		if (tr) {
			tr->SetPos(Vector3(-1.f, 0.f, 0.f));
		}

		tr = m_rose->GetComponent<TransformComponent>();
		if (tr) {
			//tr->SetPos(Vector3(0.f, -1.f, 0.f));
		}

		tr = m_spanwer->GetComponent<TransformComponent>();
		if (tr) {
			Vector3 pos = tr->GetPos();
			pos.z += 4.f;
			tr->SetPos(pos);
		}

		AppBase::GetInputManager().BindInputAction(m_lButton, InputState::Pressed, this, &ParticleEditor::ClickEvent);
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
		ClickEffectManager::Get().TriggerPreset("Smoke");
		
		// 또는 직접 Scene에서 생성
		// InputManager& inputMgr = AppBase::GetInputManager();
		// Vector2 mouseNDC = inputMgr.GetMouseNDC();
		// Vector3 worldPos(mouseNDC.x * 10.0f, 0.0f, mouseNDC.y * 10.0f);
		// SpawnEffect<EffectActor>(L"ClickEffect", L"Particles\\SmokeEffect.json", worldPos);
	}
}