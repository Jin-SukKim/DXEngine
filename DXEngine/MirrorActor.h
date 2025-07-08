#pragma once
#include "Actor.h"

namespace DE {
	class ModelComponent;
	class BoundComponent;
	class SkyboxActor;

	class MirrorActor : public Actor
	{
		using Super = Actor;
	public:
		MirrorActor(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const std::wstring& name);
		~MirrorActor() override {}

		void Initialize() override;
		void Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime) override;
		void Render(RenderBase& renderer, std::vector<std::shared_ptr<Actor>>* actorList, std::shared_ptr<SkyboxActor>& cubeMap);
		void Render(RenderBase& renderer) override;

		void SetGlobals(ComPtr<ID3D11DeviceContext>& context);
		void UpdateGlobalConstants(ComPtr<ID3D11DeviceContext>& context, const GlobalConstants& globalConstsCPU, const float& deltaTime, const Vector3& eyeWorld, const Matrix& view, const Matrix& proj);
	private:
		ModelComponent* m_mirror; // 거울 형상
		DirectX::SimpleMath::Plane m_mirrorPlane; // 반사 행렬을 만들기 위한 Plane(일종의 반사되는 기준)
		DirectX::SimpleMath::Plane m_reflectPlane;
		float m_mirrorAlpha = 0.15f; // 거울 자체의 Alpha로 1.0에 가까울수록 반사가 안됨

		// 거울 반사를 위한 공통 Data
		ConstantBuffer<GlobalConstants> m_reflectGlobalConsts;


		BoundComponent* m_boundVolume;
	};
}
