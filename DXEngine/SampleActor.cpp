#include "pch.h"
#include "SampleActor.h"

#include "GeometryGenerator.h"
#include "D3D11Utils.h"
#include "TransformComponent.h"
#include "ModelComponent.h"

namespace DE {
	SampleActor::SampleActor(ComPtr<ID3D11Device>& device, const std::wstring& name) : Super(device, name)
	{
		// main object
		{
			std::vector<MeshData> meshes = GeometryGenerator::ReadFromFile("../Assets/Characters/Zelda/source/", "zeldaPosed001.fbx");
			m_gelda = AddComponent<ModelComponent>(device, L"Model");
			m_gelda->SetModel(device, meshes);
		}
	}
	void SampleActor::Initialize() {
		Super::Initialize();

		m_gelda->SetDrawNormal(true);
	}

	void SampleActor::Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime) {
		Super::Update(context, deltaTime);
		// constant buffer data °»½Å
		TransformComponent* tr = this->GetComponent<TransformComponent>();
		if (tr) {
			tr->RotateYaw(0.05f);
			tr->RotatePitch(0.05f);
		}
	}

	void SampleActor::Render(ComPtr<ID3D11DeviceContext>& context) {
		Super::Render(context);
	}

	void SampleActor::RenderNormal(ComPtr<ID3D11DeviceContext>& context)
	{
		m_gelda->RenderNormal(context);
	}

	bool SampleActor::IsDrawNormal()
	{
		return m_gelda->IsDrawNormal();
	}
}