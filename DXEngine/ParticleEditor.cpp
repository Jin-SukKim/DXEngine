#include "pch.h"
#include "ParticleEditor.h"
#include "ParticleEmitter.h"
#include "SquareActor.h"

// https://dev.epicgames.com/documentation/en-us/unreal-engine/particle-system-user-guide?application_version=4.27
namespace DE {
	ParticleEditor::ParticleEditor() : Scene()
	{
		ground = AddObject<SquareActor>(L"Ground");
		particleEmitter = std::make_unique<ParticleEmitter>(L"Particle");
		particleEmitter->SetupFire();
		//particleEmitter->SetupExplosion();
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