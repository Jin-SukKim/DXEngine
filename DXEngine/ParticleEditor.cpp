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
#include "GeometryGenerator.h"
#include "ModelManager.h"

// https://dev.epicgames.com/documentation/en-us/unreal-engine/particle-system-user-guide?application_version=4.27
namespace DE {
	ParticleEditor::ParticleEditor() : Scene()
	{
		ground = AddObject<SquareActor>(L"Ground");

		effect = AddEffect<ParticleSystem>(L"Effect");
		ParticleLoader::Load<ParticleSystem>(L"Particles\\FireEffect.json", effect);
 		
		MeshData box = GeometryGenerator::MakeBox();
		ModelManager::Get().LoadModel("ParticleBox", box);
	}

	ParticleEditor::~ParticleEditor()
	{
	}

	void ParticleEditor::Initialize()
	{	
		Scene::Initialize();
	}

	void ParticleEditor::Update(const float& dt)
	{
		Scene::Update(dt);

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
	}
}