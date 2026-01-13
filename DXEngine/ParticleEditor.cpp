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

// https://dev.epicgames.com/documentation/en-us/unreal-engine/particle-system-user-guide?application_version=4.27
namespace DE {
	ParticleEditor::ParticleEditor() : Scene()
	{
		ground = AddObject<SquareActor>(L"Ground");
 		particleEmitter = ParticleLoader::Load(L"Fire.json");
 		particleEmitter2 = ParticleLoader::Load(L"Smoke.json");
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

		particleEmitter->Initialize();
		particleEmitter2->Initialize();
	}

	void ParticleEditor::Update(const float& dt)
	{
		Scene::Update(dt);

		particleEmitter->Update(dt);
		particleEmitter2->Update(dt);

		FileWatcher::Get().Update(); // File이 변하는지 감시
	}

	void ParticleEditor::UpdateGUI()
	{
		Scene::UpdateGUI();
	}

	void ParticleEditor::Render()
	{
		Scene::Render();
		
		particleEmitter->Render(m_globalConstsGPU);
		particleEmitter2->Render(m_globalConstsGPU);
	}
}