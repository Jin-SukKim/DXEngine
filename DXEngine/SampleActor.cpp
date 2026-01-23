#include "pch.h"
#include "SampleActor.h"

#include "GeometryGenerator.h"
#include "D3D11Utils.h"
#include "TransformComponent.h"
#include "ModelComponent.h"

#include "RenderBase.h"
#include "ParticleSystemComponent.h"

namespace DE {
	SampleActor::SampleActor(const std::wstring& name) : Super(name)
	{
		// main object
		{
			m_sample = AddComponent<ModelComponent>(L"Model");
			m_sample->SetModel("DamagedHelmet.gltf", "../Assets/Models/DamagedHelmet/", true);

			m_particles = AddComponent<ParticleSystemComponent>(L"Particles");
			m_particles->SetSystem(L"Particles\\TestEffect.json", &meshes[0]);
			
			//MeshData box = GeometryGenerator::MakeBox();
			//ModelManager::Get().LoadModel("ParticleBox", box);
		}
	}
	void SampleActor::Initialize() {
		Super::Initialize();

		m_sample->SetDrawNormal(false);

		TransformComponent* tr = this->GetComponent<TransformComponent>();
		if (tr) {
			tr->SetPos(Vector3(0.0f, 0.5f, 0.0f));
			//tr->SetScale(Vector3(5.f));
		}
	}

	void SampleActor::Update(const float& deltaTime) {
		Super::Update(deltaTime);
		// constant buffer data °»½Å
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