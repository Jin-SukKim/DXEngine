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
#include "TransformComponent.h"

// https://dev.epicgames.com/documentation/en-us/unreal-engine/particle-system-user-guide?application_version=4.27
namespace DE {
	ParticleEditor::ParticleEditor() : Scene()
	{
		ground = AddObject<SquareActor>(L"Ground");
 		//particleEmitter = ParticleLoader::Load<ParticleEmitter>(L"Emitters\\Fire.json");
 		//particleEmitter2 = ParticleLoader::Load<ParticleEmitter>(L"Emitters\\Smoke.json");

		effect = AddEffect<ParticleSystem>(L"Effect");
		ParticleLoader::Load<ParticleSystem>(L"FireEffect.json", effect);
		//effect = std::make_unique<ParticleSystem>(L"Effect");
		//effect->AddEmitter(std::move(particleEmitter));
		//effect->AddEmitter(std::move(particleEmitter2));
 	}

	ParticleEditor::~ParticleEditor()
	{
	}

	void ParticleEditor::Initialize()
	{
		Scene::Initialize();

		TransformComponent* tr = effect->GetComponent<TransformComponent>();
		if (tr) {
			tr->SetPos(Vector3(0.f, 0.5f, 0.f));
		}
		//effect->Initialize();
		effect->OnSpawn();	
	}

	void ParticleEditor::Update(const float& dt)
	{
		Scene::Update(dt);
		//effect->Update(dt);

		TransformComponent* tr = effect->GetComponent<TransformComponent>();
		if (tr) {
			Vector3 pos = tr->GetPos();
			pos = Vector3::Transform(pos, Matrix::CreateRotationZ(dt * 1.0f));
			tr->SetPos(pos);
		}

		FileWatcher::Get().Update(); // File이 변하는지 감시
	}

	void ParticleEditor::UpdateGUI()
	{
		Scene::UpdateGUI();
	}

	void ParticleEditor::Render()
	{
		Scene::Render();
		//RenderBase& renderer = *GET_SINGLE(RenderBase);
		//renderer.SetPipelineState(RenderBase::graphicsCommon.particle.animPSO);
		//effect->Render();
	}
}