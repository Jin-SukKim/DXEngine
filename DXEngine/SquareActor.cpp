#include "pch.h"
#include "SquareActor.h"

#include "GeometryGenerator.h"
#include "D3D11Utils.h"
#include "TransformComponent.h"
#include "ModelComponent.h"

#include "RenderBase.h"

namespace DE {
	SquareActor::SquareActor(const std::wstring& name) : Super(name)
	{
		// main object
		{
			MeshData mesh = GeometryGenerator::MakeSquare(3.f);

			m_sample = AddComponent<ModelComponent>(L"Model");
			m_sample->SetModel("Box", mesh);
		}
	}

	void SquareActor::Initialize() {
		Super::Initialize();

		m_sample->SetDrawNormal(false);

		TransformComponent* tr = this->GetComponent<TransformComponent>();
		if (tr) {
			tr->SetPos({ 0.f, -2.5f, 0.f });
			tr->SetLocalRotation(0.f, 90.f, 0.f);
			tr->SetScale(Vector3(10.f));
		}
	}

	void SquareActor::Update(const float& deltaTime) {
		Super::Update(deltaTime);
	}

	void SquareActor::Render() {
		Super::Render();
	}

}