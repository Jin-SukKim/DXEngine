#include "pch.h"
#include "MirrorActor.h"
#include "RenderBase.h"
#include "ModelComponent.h"
#include "GeometryGenerator.h"
#include "TransformComponent.h"
#include "SkyboxActor.h"

namespace DE {
	MirrorActor::MirrorActor(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const std::wstring& name) : Super(device, context, name)
	{
		MeshData mesh = GeometryGenerator::MakeSquare(1.f);
		m_mirror = AddComponent<ModelComponent>(device, L"Mirror");
		m_mirror->SetModel(device, context, mesh);

		MaterialConstants consts = m_mirror->GetMaterialCpu();
		consts.albedoFactor = Vector3(0.3f);
		consts.emissionFactor = Vector3(0.0f);
		consts.metallicFactor = 0.7f;
		consts.roughnessFactor = 0.2f;
		m_mirror->SetMaterial(consts);

		Vector3 pos = Vector3(0.f, 0.f, 0.f);
		Vector3 normal = Vector3(0.f, 0.f, -1.f);
		m_mirrorPlane = DirectX::SimpleMath::Plane(pos, normal);

		m_reflectGlobalConsts.Initialize(device);
	}

	void MirrorActor::Initialize()
	{
		Super::Initialize();
	}

	void MirrorActor::Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime)
	{
		Super::Update(context, deltaTime);

		TransformComponent* tr = this->GetComponent<TransformComponent>();
		if (tr) {
			Matrix mirrorTransform = tr->GetRotationMatrix() * tr->GetTranslateMatrix();
			m_reflectPlane = m_mirrorPlane.Transform(m_mirrorPlane, mirrorTransform);
		}
	}

	void MirrorActor::Render(RenderBase& renderer, std::vector<std::shared_ptr<Actor>>& actorList, std::shared_ptr<SkyboxActor>& cubeMap)
	{
		ComPtr<ID3D11DeviceContext> context = renderer.GetContext();
		
		// 거울 위치만 StencilBuffer에 1로 표기
		// 거울을 가리는 물체가 있을 수도 있기 때문에 Depth는 Clear하지 않음
		// 기본 물체 렌더링할 때 drawDSS에서 모두 KEEP 설정만 사용했기 때문에
		// 굳이 Stencil Buffer를 Clear해주지 않아도 됨
		renderer.ClearStencilBuffer(); 

		cubeMap->SetCommonSRVToNull(context); // IBL용 Shader Resource를 NULL로 설정
		// 거울을 렌더링 과정을 통해 Stencil Buffer에 Masking
		renderer.SetPipelineState(RenderBase::graphicsCommon.mirror.stencilMaskPSO);
		RenderComponent(context, ComponentType::Model);
		
		// 거울 부분은 다른 반사된 세상을 그리므로 거울 부분의 Depth를 초기화
		renderer.ClearDepthBuffer();

		// 거울 위치에 반사된 물체들을 렌더링
		SetGlobals(context);
		// 반사된 세상을 그리므로 winding이 바뀜
		cubeMap->SetCommonSRVs(context);
		// 반사된 세상을 물체들을 렌더링
		for (auto& model : actorList)
			if (model.get() != this)
				model->Render(renderer, true);
			
		// 반사된 세상의 환경맵 렌더링
		cubeMap->Render(renderer, true);

		// 거울 자체의 재질을 Blend로 렌더링
		const float t = 1.f - m_mirrorAlpha;
		const float blendFactor[4] = { t, t, t, 1.f };
		RenderBase::graphicsCommon.mirror.mirrorBlendSolidPSO.SetBlendFactor(blendFactor);
		cubeMap->SetCommonSRVToNull(context);
		renderer.SetPipelineState(RenderBase::graphicsCommon.mirror.mirrorBlendSolidPSO);
		RenderComponent(context, ComponentType::Model);

		renderer.SetPipelineState(RenderBase::graphicsCommon.basic.boundPSO);
		RenderComponent(context, ComponentType::BoundingVolume);
	}

	void MirrorActor::SetGlobals(ComPtr<ID3D11DeviceContext>& context)
	{
		// Global Constants을 Shader에서 사용할 수 있도록 설정
		context->VSSetConstantBuffers(0, 1, m_reflectGlobalConsts.GetAddressOf());
		context->GSSetConstantBuffers(0, 1, m_reflectGlobalConsts.GetAddressOf());
		context->PSSetConstantBuffers(0, 1, m_reflectGlobalConsts.GetAddressOf());
	}

	void MirrorActor::UpdateGlobalConstants(ComPtr<ID3D11DeviceContext>& context, const GlobalConstants& globalConstsCPU, const float& deltaTime, const Vector3& eyeWorld, const Matrix& view, const Matrix& proj)
	{
		Matrix reflect = Matrix::CreateReflection(m_reflectPlane);
		// DirectX는 Row-Major인데 HLSL는 Column-Major이므로 Transpose
		// Reflection
		m_reflectGlobalConsts.GetCpu() = globalConstsCPU;
		memcpy(&m_reflectGlobalConsts.GetCpu(), &globalConstsCPU, sizeof(globalConstsCPU));
		m_reflectGlobalConsts.GetCpu().view = (reflect * view).Transpose();
		m_reflectGlobalConsts.GetCpu().viewProj = (reflect * view * proj).Transpose();

		m_reflectGlobalConsts.Upload(context);
	}
}