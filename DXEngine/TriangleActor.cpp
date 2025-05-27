#include "pch.h"
#include "TriangleActor.h"

#include "GeometryGenerator.h"
#include "D3D11Utils.h"
#include "TransformComponent.h"
#include "ModelComponent.h"

namespace DE {
	TriangleActor::TriangleActor(ComPtr<ID3D11Device>& device, const std::wstring& name) : Super(device, name)
	{
		// main object
		{
			std::vector<MeshData> meshes = GeometryGenerator::ReadFromFile("../Assets/Characters/Zelda/source/", "zeldaPosed001.fbx");
			m_gelda = AddComponent<ModelComponent>(device, L"Model");
			m_gelda->SetModel(device, meshes);
		}
	}
	void TriangleActor::Initialize() {
		Super::Initialize();

		m_gelda->SetDrawNormal(true);
	}

	void TriangleActor::Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime) {
		Super::Update(context, deltaTime);
		// constant buffer data °»½Å
		TransformComponent* tr = this->GetComponent<TransformComponent>();
		if (tr) {
			tr->RotateYaw(0.05);
			tr->RotatePitch(0.05);
		}
	}

	void TriangleActor::Render(ComPtr<ID3D11DeviceContext>& context) {
		Super::Render(context);
	}

	void TriangleActor::RenderNormal(ComPtr<ID3D11DeviceContext>& context)
	{
		m_gelda->RenderNormal(context);
	}

	bool TriangleActor::IsDrawNormal()
	{
		return m_gelda->IsDrawNormal();
	}
}