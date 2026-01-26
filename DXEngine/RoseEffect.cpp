#include "pch.h"
#include "RoseEffect.h"
#include "ParticleManager.h"
#include "ParticleSystem.h"
#include "ModelComponent.h"
#include "TransformComponent.h"
#include "MaterialSystem.h"

namespace DE {
RoseEffect::RoseEffect(const std::wstring& name) : Super(name)
{
	m_model = AddComponent<ModelComponent>(L"Model");
	// basePath만 전달 (presetPath는 사용하지 않음)
	m_model->SetModel("angel_armor.fbx", "armored-female-future-soldier/", false);
	m_model->SetDrawNormal(false);

	int matIdx = MaterialSystem::Get().CreateMaterialFromJson("Materials\\angel_armor0.json");
	m_model->SetMaterialIndex(matIdx);

	m_orbit = ParticleManager::Get().CreateSystem(L"Particles\\OrbitEffect.json");
	int modelIdx = m_model->GetModelIndex();
	m_orbit->SetTarget(this, modelIdx);
}

RoseEffect::~RoseEffect()
{
	ParticleManager::Get().DestroyInstance(m_orbit);
}

void RoseEffect::Initialize()
{
	Super::Initialize();

	TransformComponent* tr = this->GetComponent<TransformComponent>();
	if (tr) {
		//tr->SetPos(Vector3(0.0f, 0.5f, 0.0f));
		//tr->SetScale(Vector3(0.009f));
	}
}

void RoseEffect::Update(const float& deltaTime)
{
	Super::Update(deltaTime);
}

void RoseEffect::Render()
{
	Super::Render();
}

}
