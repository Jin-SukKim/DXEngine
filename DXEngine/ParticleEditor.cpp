#include "pch.h"
#include "ParticleEditor.h"

#include "SquareActor.h"
#include "PointLight.h"
namespace DE {
	ParticleEditor::ParticleEditor() : Scene()
	{
		m_ground = AddObject<SquareActor>(L"Ground");
		m_pointLight = AddLight<PointLight>(L"Light");
	}

	ParticleEditor::~ParticleEditor()
	{
	}

	void ParticleEditor::Initialize()
	{
		Scene::Initialize();
	}

	void ParticleEditor::Update(const float& deltaTime)
	{
		Scene::Update(deltaTime);
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