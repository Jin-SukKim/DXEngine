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
		// m_particle θ ~EffectActor() 
	}

	void BoxMeshEffect::Initialize() {
		Super::Initialize();
	}

	void BoxMeshEffect::Update(const float& deltaTime) {
		Super::Update(deltaTime);
		// constant buffer data 
		TransformComponent* tr = this->GetComponent<TransformComponent>();
		if (tr) {
			Vector3 cur = tr->GetPos();
			if (cur.x < -36.f)
				tr->SetPos(Vector3(-32.f, 0.f, -8.f));
			Vector3 move = Vector3(-1.5f, 0.f, 0.0f) * deltaTime;
			tr->SetPos(tr->GetPos() + move);
		}
	}
}