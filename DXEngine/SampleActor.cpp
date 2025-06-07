#include "pch.h"
#include "SampleActor.h"

#include "GeometryGenerator.h"
#include "D3D11Utils.h"
#include "TransformComponent.h"
#include "ModelComponent.h"
#include "BoundComponent.h"
#include "RenderBase.h"

namespace DE {
	SampleActor::SampleActor(ComPtr<ID3D11Device>& device, const std::wstring& name) : Super(device, name)
	{
		// main object
		{
			std::vector<MeshData> meshes = GeometryGenerator::ReadFromFile("../Assets/Characters/Zelda/source/", "zeldaPosed001.fbx");
			m_gelda = AddComponent<ModelComponent>(device, L"Model");
			m_gelda->SetModel(device, meshes);

			m_boundVolume = AddComponent<BoundComponent>(device, L"BoundingVolume");
			m_boundVolume->SetBoundingVolume(device, meshes);
		}
	}
	void SampleActor::Initialize() {
		Super::Initialize();

		m_gelda->SetDrawNormal(false);
	}

	void SampleActor::Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime) {
		Super::Update(context, deltaTime);
		// constant buffer data 갱신
		TransformComponent* tr = this->GetComponent<TransformComponent>();
		if (tr) {
			//tr->LocalRotate(0.1f, 0.1f, 0.1f);
			//tr->RotateYaw(0.05f);
			//tr->RotatePitch(0.05f);
		}
	}

	void SampleActor::Render(RenderBase& renderer) {
		Super::Render(renderer);

		if (IsDrawNormal()) {
			// Normal Vector 그리기
			renderer.SetPipelineState(RenderBase::graphicsCommon.normal.solidPSO);
			RenderNormal(renderer.GetContext());
		}
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