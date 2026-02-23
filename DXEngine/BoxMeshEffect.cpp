#include "pch.h"
#include "BoxMeshEffect.h"
#include "GeometryGenerator.h"
#include "TransformComponent.h"
#include "ModelComponent.h"

namespace DE {
	BoxMeshEffect::BoxMeshEffect(const std::wstring& name) : Super(name)
	{
		{
			MeshData cubeMap = GeometryGenerator::MakeBox(1.f);
			m_sample = AddComponent<ModelComponent>(name);
			m_sample->SetModel("BoxMesh", cubeMap);
		}

		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\BoxMesh\\BoxMesh.json");

		if (m_particle) {
			int modelIdx = m_sample->GetModelIndex();
			m_particle->SetTarget(this, modelIdx);
		}
	}

	BoxMeshEffect::~BoxMeshEffect()
	{
		// m_particle은 부모 ~EffectActor()에서 해제됨
	}

	void BoxMeshEffect::Initialize() {
		Super::Initialize();


		TransformComponent* tr = this->GetComponent<TransformComponent>();
		if (tr) {
			//tr->SetPos(Vector3(0.0f, 0.5f, 0.0f));
			//tr->SetScale(Vector3(0.5f));
		}
	}

	void BoxMeshEffect::Update(const float& deltaTime) {
		Super::Update(deltaTime);
		// constant buffer data 갱신
		TransformComponent* tr = this->GetComponent<TransformComponent>();
		if (tr) {
			tr->SetPos(tr->GetPos() + Vector3(3.f, 0.f, 0.f) * deltaTime);
		}
	}
}