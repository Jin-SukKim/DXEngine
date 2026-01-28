#include "pch.h"
#include "SampleActor.h"

#include "GeometryGenerator.h"
#include "D3D11Utils.h"
#include "TransformComponent.h"
#include "ModelComponent.h"

#include "RenderBase.h"
#include "ParticleSystem.h"
#include "ModelManager.h"
#include "ParticleManager.h"

namespace DE {
	SampleActor::SampleActor(const std::wstring& name) : Super(name)
	{
		// main object
		{
			m_sample = AddComponent<ModelComponent>(L"Model");
			// basePath만 전달 (presetPath는 사용하지 않음)
			m_sample->SetModel("DamagedHelmet.gltf", "DamagedHelmet/", true);

			m_particles = ParticleManager::Get().CreateSystem(L"Particles\\TestEffect.json");

			if (m_particles) {
				int modelIdx = m_sample->GetModelIndex();
				m_particles->SetTarget(this, modelIdx);
			}
			m_sample->SetDrawNormal(false);
		}
	}

	// 소멸자 추가
	SampleActor::~SampleActor()
	{
		if (m_particles) {
			ParticleManager::Get().DestroyInstance(m_particles);
			m_particles = nullptr;
		}
	}

	void SampleActor::Initialize() {
		Super::Initialize();


		TransformComponent* tr = this->GetComponent<TransformComponent>();
		if (tr) {
			//tr->SetPos(Vector3(0.0f, 0.5f, 0.0f));
			//tr->SetScale(Vector3(0.5f));
		}
	}

	void SampleActor::Update(const float& deltaTime) {
		Super::Update(deltaTime);
		// constant buffer data 갱신
		TransformComponent* tr = this->GetComponent<TransformComponent>();
		if (tr) {
			//tr->LocalRotate(0.1f, 0.1f, 0.1f);
			//tr->RotateYaw(0.05f);
			//tr->RotatePitch(0.05f);
		}
	}

	void SampleActor::Render() {
		Super::Render();
	}


}