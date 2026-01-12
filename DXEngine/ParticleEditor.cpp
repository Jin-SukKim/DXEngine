#include "pch.h"
#include "ParticleEditor.h"
#include "ParticleEmitter.h"
#include "SquareActor.h"
#include "EmitterModule.h"
#include "VisualModule.h"
#include "ForceModule.h"
#include "RenderModule.h"

// https://dev.epicgames.com/documentation/en-us/unreal-engine/particle-system-user-guide?application_version=4.27
namespace DE {
	ParticleEditor::ParticleEditor() : Scene()
	{
		ground = AddObject<SquareActor>(L"Ground");
		particleEmitter = std::make_unique<ParticleEmitter>(L"Particle");

		std::unique_ptr emitter = std::make_unique<EmitterModule>();
		particleEmitter->AddModule<EmitterModule>(std::move(emitter));

		std::unique_ptr visual = std::make_unique<VisualModule>();
		particleEmitter->AddModule<VisualModule>(std::move(visual));

		std::unique_ptr force = std::make_unique<ForceModule>();
		particleEmitter->AddModule<ForceModule>(std::move(force));
		
		std::unique_ptr render = std::make_unique<RenderModule>();
		particleEmitter->AddModule<RenderModule>(std::move(render));
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