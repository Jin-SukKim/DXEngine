#pragma once
#include "Actor.h"

namespace DE {
	class ModelComponent;

	class SkyboxActor : public Actor 
	{
		using Super = Actor;
	public:
		SkyboxActor(ComPtr<ID3D11Device>& device, const std::wstring& name);
		~SkyboxActor() override {}

		void Initialize() override;
		void Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime) override;
		void Render(RenderBase& renderer) override;

		void SetCubeMaps(ComPtr<ID3D11Device>& device, std::wstring basePath, std::wstring envFilename, std::wstring specularFilename, std::wstring irradianceFilename, std::wstring brdfFilename);
		
		// 렌더링할 때 공통으로 사용할 Texture들을 Graphics Pipeline에 설정
		void SetCommonSRVs(ComPtr<ID3D11DeviceContext>& context);
	private:
		ModelComponent* m_sky;

		// Cubemap은 Shader에서 읽어서 사용만 하면 되므로 SRV만 있으면 됨
		ComPtr<ID3D11ShaderResourceView> m_envSRV; // Cubemap
		ComPtr<ID3D11ShaderResourceView> m_specularSRV; // Specular (아주 선명한 Texture)
		ComPtr<ID3D11ShaderResourceView> m_irradianceSRV; // Diffuse (부드럽게 전처리한 Texture)
		ComPtr<ID3D11ShaderResourceView> m_brdfSRV;
	};
}
