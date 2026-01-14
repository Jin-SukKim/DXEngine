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
#include "ParticleSystem.h"

// https://dev.epicgames.com/documentation/en-us/unreal-engine/particle-system-user-guide?application_version=4.27
namespace DE {
	ParticleEditor::ParticleEditor() : Scene()
	{
		ground = AddObject<SquareActor>(L"Ground");
 		particleEmitter = ParticleLoader::Load<ParticleEmitter>(L"Fire.json");
 		particleEmitter2 = ParticleLoader::Load<ParticleEmitter>(L"Smoke.json");

		effect = AddObject<ParticleSystem>(L"Effect");
		effect->AddEmitter(std::move(particleEmitter));
		effect->AddEmitter(std::move(particleEmitter2));
 		
		//particleEmitter = ParticleLoader::Load(L"VortexAura.json");
		//particleEmitter = std::make_unique<ParticleEmitter>(L"Particle");
		//particleEmitter->AddModule(ParticleModuleFactory::Create("Spawn"));
		//particleEmitter->AddModule(ParticleModuleFactory::Create("Visual"));
		//particleEmitter->AddModule(ParticleModuleFactory::Create("Force"));
		//particleEmitter->AddModule(ParticleModuleFactory::Create("BillboardRender"));
	}

	ParticleEditor::~ParticleEditor()
	{
	}

	void ParticleEditor::Initialize()
	{
		Scene::Initialize();
		effect->Initialize();
		effect->OnSpawn();
		//particleEmitter->Initialize();
		//particleEmitter2->Initialize();

		//particleEmitter->OnSpawn();
		//particleEmitter2->OnSpawn();
	}

	void ParticleEditor::Update(const float& dt)
	{
		Scene::Update(dt);
		effect->Update(dt);
		//particleEmitter->Update(dt);
		//particleEmitter2->Update(dt);
		FileWatcher::Get().Update(); // File이 변하는지 감시
	}

	void ParticleEditor::UpdateGUI()
	{
		Scene::UpdateGUI();
	}

	void ParticleEditor::Render()
	{
		Scene::Render();
		effect->Render();
		//particleEmitter->Render();
		//particleEmitter2->Render();
		
	}
}