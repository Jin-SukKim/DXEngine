#include "pch.h"
#include "MirrorActor.h"
#include "RenderBase.h"
#include "ModelComponent.h"
#include "GeometryGenerator.h"
#include "TransformComponent.h"
#include "SkyboxActor.h"
#include "BoundComponent.h"

namespace DE {
	MirrorActor::MirrorActor(const std::wstring& name) : Super(name)
	{
		MeshData mesh = GeometryGenerator::MakeSquare(1.f);
		m_mirror = AddComponent<ModelComponent>(L"Mirror");
		m_mirror->SetModel(mesh);

		m_boundVolume = AddComponent<BoundComponent>(L"BoundingVolume");
		m_boundVolume->SetBoundingVolume({ mesh });

		MaterialConstants consts = m_mirror->GetMaterialCpu();
		consts.albedoFactor = Vector3(0.3f);
		consts.emissionFactor = Vector3(0.0f);
		consts.metallicFactor = 0.7f;
		consts.roughnessFactor = 0.3f;
		m_mirror->SetMaterial(consts);

		Vector3 pos = Vector3(0.f, 0.f, 0.f);
		Vector3 normal = Vector3(0.f, 0.f, -1.f);
		m_mirrorPlane = DirectX::SimpleMath::Plane(pos, normal);

		m_reflectGlobalConsts.Initialize();
	}

	void MirrorActor::Initialize()
	{
		Super::Initialize();

		m_boundVolume->SetVisibility(false);
	}

	void MirrorActor::Update(const float& deltaTime)
	{
		Super::Update(deltaTime);

		TransformComponent* tr = this->GetComponent<TransformComponent>();
		if (tr) {
			Matrix mirrorTransform = tr->GetRotationMatrix() * tr->GetTranslateMatrix();
			m_reflectPlane = m_mirrorPlane.Transform(m_mirrorPlane, mirrorTransform);
		}
	}

	void MirrorActor::Render(std::vector<std::shared_ptr<Actor>>* actorList, std::shared_ptr<SkyboxActor>& cubeMap, const ComPtr<ID3D11Buffer>& globalConstsGPU)
	{
		if (IsVisible() == false)
			return;

		RenderBase& renderer = *GET_SINGLE(RenderBase);
		ComPtr<ID3D11DeviceContext> context = renderer.GetContext();

		// 거울 위치만 StencilBuffer에 1로 표기
		// 거울을 가리는 물체가 있을 수도 있기 때문에 Depth는 Clear하지 않음
		// 기본 물체 렌더링할 때 drawDSS에서 모두 KEEP 설정만 사용했기 때문에
		// 굳이 Stencil Buffer를 Clear해주지 않아도 됨
		renderer.ClearStencilBuffer(); 

		cubeMap->SetCommonSRVToNull(); // IBL용 Shader Resource를 NULL로 설정

		// 거울을 렌더링 과정을 통해 Stencil Buffer에 Masking
		renderer.SetPipelineState(RenderBase::graphicsCommon.mirror.stencilMaskPSO);
		RenderComponent(ComponentType::Model);
		
		// 거울 부분은 다른 반사된 세상을 그리므로 거울 부분의 Depth를 초기화
		renderer.ClearDepthBuffer();

		// 거울 위치에 반사된 물체들을 렌더링
		SetGlobals(m_reflectGlobalConsts.Get());
		// 반사된 세상을 그리므로 winding이 바뀜
		cubeMap->SetCommonSRVs();
		// 반사된 세상을 물체들을 렌더링
		renderer.SetPipelineState(RenderBase::graphicsCommon.mirror.reflectSolidPSO);
		for (auto& model : actorList[0])
			if (model.get() != this)
				model->Render();

		renderer.SetPipelineState(RenderBase::graphicsCommon.mirror.reflectBillboardSolidPSO);
		for (auto& billboard : actorList[1])
			billboard->Render();
			
		// 반사된 세상의 환경맵 렌더링
		renderer.SetPipelineState(RenderBase::graphicsCommon.mirror.reflectSkyboxSolidPSO);
		cubeMap->Render();

		// 거울 자체의 재질을 Blend로 렌더링
		const float t = 1.f - m_mirrorAlpha;
		const float blendFactor[4] = { t, t, t, 1.f };
		RenderBase::graphicsCommon.mirror.mirrorBlendSolidPSO.SetBlendFactor(blendFactor);
		cubeMap->SetCommonSRVToNull();
		renderer.SetPipelineState(RenderBase::graphicsCommon.mirror.mirrorBlendSolidPSO);
		SetGlobals(globalConstsGPU);
		RenderComponent(ComponentType::Model);

		renderer.SetPipelineState(RenderBase::graphicsCommon.basic.boundPSO);
		RenderComponent(ComponentType::BoundingVolume);
	}

	void MirrorActor::Render()
	{
		Super::Render();
	}

	void MirrorActor::SetGlobals(const ComPtr<ID3D11Buffer>& globalConstsGPU)
	{
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		// Global Constants을 Shader에서 사용할 수 있도록 설정
		context->VSSetConstantBuffers(0, 1, globalConstsGPU.GetAddressOf());
		context->GSSetConstantBuffers(0, 1, globalConstsGPU.GetAddressOf());
		context->PSSetConstantBuffers(0, 1, globalConstsGPU.GetAddressOf());
	}

	void MirrorActor::UpdateGlobalConstants(const GlobalConstants& globalConstsCPU, const float& deltaTime, const Vector3& eyeWorld, const Matrix& view, const Matrix& proj)
	{
		Matrix reflect = Matrix::CreateReflection(m_reflectPlane);
		// DirectX는 Row-Major인데 HLSL는 Column-Major이므로 Transpose
		// Reflection
		m_reflectGlobalConsts.GetCpu() = globalConstsCPU;
		memcpy(&m_reflectGlobalConsts.GetCpu(), &globalConstsCPU, sizeof(globalConstsCPU));
		m_reflectGlobalConsts.GetCpu().view = (reflect * view).Transpose();
		m_reflectGlobalConsts.GetCpu().viewProj = (reflect * view * proj).Transpose();
		m_reflectGlobalConsts.GetCpu().invProj = proj.Invert().Transpose();
		
		// 그림자 렌더링에 사용 (거울의 경우 광원의 위치도 반사시킨 후에 계산해야 함)
		m_reflectGlobalConsts.GetCpu().invViewProj = m_reflectGlobalConsts.GetCpu().viewProj.Invert();

		m_reflectGlobalConsts.Upload();
	}
}