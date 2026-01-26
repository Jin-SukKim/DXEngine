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

// https://dev.epicgames.com/documentation/en-us/unreal-engine/particle-system-user-guide?application_version=4.27
namespace DE {
	ParticleEditor::ParticleEditor() : Scene(), m_click(m_lButton)
	{
		ParticleModuleFactory::Register<SpawnModule>("Spawn");
		ParticleModuleFactory::Register<VisualModule>("Visual");
		ParticleModuleFactory::Register<ForceModule>("Force");
		ParticleModuleFactory::Register<VortexModule>("Vortex");
		ParticleModuleFactory::Register<BillboardRenderModule>("BillboardRender");
		ParticleModuleFactory::Register<MaterialModule>("Material");
		ParticleModuleFactory::Register<MeshRenderModule>("MeshRender");

		ClickEffectManager::Get().Initialize();
		ground = AddObject<SquareActor>(L"Ground");
		
		//for (int y = 0; y < 2; ++y) {
		//	for (int x = 0; x < 10; ++x) {
		//		EffectActor* effect = AddObject<EffectActor>(L"Effect" + x);
		//		effects.emplace_back(effect);
		//	}
		//}

		//m_spanwer = AddObject<ParticleSpawner>(L"TempSpawner");
		//m_spanwer->SetParticlePreset(L"Particles\\TempEffect.json");
		//m_spanwer->SetSpawnMode(SpawnMode::Interval); // or SpawnMode::Continuous
		//m_spanwer->SetSpawnInterval(0.1f);
		//m_spanwer->SetSpawnRadius(2.0f);
		//m_spanwer->SetMaxActiveParticles(100);

		m_firework = AddObject<Firework>(L"Firework");
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

		//for (int y = 0; y < 2; ++y) {
		//	for (int x = 0; x < 10; ++x) {
		//		TransformComponent* tr = effects[y * 10 + x]->GetComponent<TransformComponent>();
		//		if (tr) {
		//			Vector3 pos = tr->GetPos() + Vector3(float(x), float(y), 0.f);
		//			tr->SetPos(pos);
		//		}
		//	}
		//}

		AppBase::GetInputManager().BindInputAction(m_lButton, InputState::Pressed, this, &ParticleEditor::ClickEvent);
	}

	void ParticleEditor::Update(const float& dt)
	{
		Scene::Update(dt);

		//TransformComponent* tr = effect->GetComponent<TransformComponent>();
		//if (tr) {
		//	Vector3 pos = tr->GetPos();
		//	pos = Vector3::Transform(pos, Matrix::CreateRotationZ(dt * 1.0f));
		//	tr->SetPos(pos);
		//}
		ClickEffectManager::Get().Update(dt);
		FileWatcher::Get().Update(); // File이 변하는지 감시
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
		ClickEffectManager::Get().TriggerPreset("fire");
	}
}