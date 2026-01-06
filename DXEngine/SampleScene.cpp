#include "pch.h"
#include "SampleScene.h"
#include "SquareActor.h"
#include "Outliner.h"
#include "DetailGui.h"
#include "PointLight.h"

namespace DE {
	SampleScene::SampleScene() : Scene()
	{
		m_ground = AddObject<SquareActor>(L"Ground");
		m_outliner = AddGui<Outliner>();
		//m_outliner->SetActorLists({ m_lights, m_actorList[0], m_actorList[1] });

		m_detailGui = AddGui<DetailGui>();

		m_pointLight = AddLight<PointLight>(L"Light");
	}

	SampleScene::~SampleScene()
	{
	}
	
	void SampleScene::Initialize()
	{
		Scene::Initialize();


	}
	void SampleScene::Update(const float& deltaTime)
	{
		Scene::Update(deltaTime);
	}
	void SampleScene::UpdateGUI()
	{
		Scene::UpdateGUI();

		if (m_outliner->GetSelectedActor()) {
			m_detailGui->SetSelectedActor(m_outliner->GetSelectedActor());
		}
	}
	void SampleScene::Render()
	{
		Scene::Render();
	}
}