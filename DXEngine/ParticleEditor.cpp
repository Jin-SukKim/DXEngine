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
	}

	void ParticleEditor::Update(const float& deltaTime)
	{
		Scene::Update(deltaTime);

		particleEmitter->Update(deltaTime);

		FileWatcher::Get().Update();
	}

	void ParticleEditor::UpdateGUI()
	{
		Scene::UpdateGUI();

		particleEmitter->UpdateGui();
	}

	void ParticleEditor::Render()
	{
		Scene::Render();
		
		particleEmitter->Render(m_globalConstsGPU);
	}
}