#include "pch.h"
#include "ParticleEditor.h"
#include "ParticleEmitter.h"
#include "SquareActor.h"

namespace DE {
	ParticleEditor::ParticleEditor() : Scene()
	{
		ground = AddObject<SquareActor>(L"Ground");
		particleEmitter = std::make_unique<ParticleEmitter>(L"Particle");
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

		particleEmitter->Render();
	}
}