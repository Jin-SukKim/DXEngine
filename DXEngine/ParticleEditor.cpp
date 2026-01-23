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
// https://dev.epicgames.com/documentation/en-us/unreal-engine/particle-system-user-guide?application_version=4.27
namespace DE {
	ParticleEditor::ParticleEditor() : Scene()
	{
		ground = AddObject<SquareActor>(L"Ground");
		
		for (int y = 0; y < 1; ++y) {
			for (int x = 0; x < 3; ++x) {
				SampleActor* effect = AddObject<SampleActor>(L"Effect" + x);
				effects.emplace_back(effect);
			}
		}
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

		for (int y = 0; y < 1; ++y) {
			for (int x = 0; x < 3; ++x) {
				TransformComponent* tr = effects[x]->GetComponent<TransformComponent>();
				if (tr) {
					Vector3 pos = tr->GetPos() + Vector3(x, y, 0.f);
					tr->SetPos(pos);
				}
			}
		}
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