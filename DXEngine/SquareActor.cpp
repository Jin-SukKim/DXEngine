#include "pch.h"
#include "SquareActor.h"

#include "GeometryGenerator.h"
#include "D3D11Utils.h"
#include "TransformComponent.h"
#include "ModelComponent.h"
#include "BoundComponent.h"
#include "RenderBase.h"

namespace DE {
	SquareActor::SquareActor(const std::wstring& name) : Super(name)
	{
		// main object
		{
			MeshData mesh = GeometryGenerator::MakeSquare(3.f);

			m_sample = AddComponent<ModelComponent>(L"Model");
			m_sample->SetModel(mesh);

			MaterialConstants consts = m_sample->GetMaterialCpu();
			consts.albedoFactor = Vector3(0.01f);
			consts.emissionFactor = Vector3(0.0f);
			consts.metallicFactor = 0.2f;
			consts.roughnessFactor = 0.8f;
			m_sample->SetMaterial(consts);
		}
	}

	void SquareActor::Initialize() {
		Super::Initialize();

		m_sample->SetDrawNormal(false);

		TransformComponent* tr = this->GetComponent<TransformComponent>();
		if (tr) {
			tr->SetPos({ 0.f, -1.f, 0.f });
			tr->SetLocalRotation(0.f, 90.f, 0.f);
		}
	}

	void SquareActor::Update(const float& deltaTime) {
		Super::Update(deltaTime);
	}

	void SquareActor::Render() {
		Super::Render();
	}

}