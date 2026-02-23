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
		// m_particle�� �θ� ~EffectActor()���� ������
	}

	void BoxMeshEffect::Initialize() {
		Super::Initialize();


		TransformComponent* tr = this->GetComponent<TransformComponent>();
		if (tr) {
			tr->SetPos(Vector3(0.0f, 0.0f, 3.0f));
			//tr->SetScale(Vector3(0.5f));
		}
	}

	void BoxMeshEffect::Update(const float& deltaTime) {
		Super::Update(deltaTime);
		// constant buffer data ����
		TransformComponent* tr = this->GetComponent<TransformComponent>();
		if (tr) {
			//m_orbitAngle += 1.f * deltaTime;
			//float radius = 1.5f;
			//tr->SetPos(Vector3(radius * cosf(m_orbitAngle), radius * sinf(m_orbitAngle), 0.f));
		}
	}
}