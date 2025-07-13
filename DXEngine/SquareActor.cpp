#include "pch.h"
#include "SquareActor.h"

#include "GeometryGenerator.h"
#include "D3D11Utils.h"
#include "TransformComponent.h"
#include "ModelComponent.h"
#include "BoundComponent.h"
#include "RenderBase.h"

namespace DE {
	SquareActor::SquareActor(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const std::wstring& name) : Super(device, context, name)
	{
		// main object
		{
			MeshData mesh = GeometryGenerator::MakeSquare(3.f);

			m_sample = AddComponent<ModelComponent>(device, L"Model");
			m_sample->SetModel(device, context, mesh);

			MaterialConstants consts = m_sample->GetMaterialCpu();
			consts.albedoFactor = Vector3(0.3f);
			consts.emissionFactor = Vector3(0.0f);
			consts.metallicFactor = 0.3f;
			consts.roughnessFactor = 0.7f;
			m_sample->SetMaterial(consts);
		}
	}

	void SquareActor::Initialize() {
		Super::Initialize();

		m_sample->SetDrawNormal(false);

		TransformComponent* tr = this->GetComponent<TransformComponent>();
		if (tr) {
			tr->SetPos({ 0.f, -1.f, 0.f });
			tr->SetRotation(0.f, 90.f, 0.f);
		}
	}

	void SquareActor::Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime) {
		Super::Update(context, deltaTime);
	}

	void SquareActor::Render(RenderBase& renderer) {
		Super::Render(renderer);
	}

}