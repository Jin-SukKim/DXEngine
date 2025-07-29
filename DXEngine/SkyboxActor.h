#pragma once
#include "Actor.h"

namespace DE {
	class ModelComponent;

	class SkyboxActor : public Actor 
	{
		using Super = Actor;
	public:
		SkyboxActor(const std::wstring& name);
		~SkyboxActor() override {}

		void Initialize() override;
		void Update(const float& deltaTime) override;
		void Render() override;

		void SetCubeMaps(std::wstring basePath, std::wstring envFilename, std::wstring specularFilename, std::wstring irradianceFilename, std::wstring brdfFilename);
		
		// 렌더링할 때 공통으로 사용할 Texture들을 Graphics Pipeline에 설정
		void SetCommonSRVs();
		// Resource로 사용하지 않기 위해 null로 설정
		void SetCommonSRVToNull();
	private:
		ModelComponent* m_sky;

		// Cubemap은 Shader에서 읽어서 사용만 하면 되므로 SRV만 있으면 됨
		ComPtr<ID3D11ShaderResourceView> m_envSRV; // Cubemap
		ComPtr<ID3D11ShaderResourceView> m_specularSRV; // Specular (아주 선명한 Texture)
		ComPtr<ID3D11ShaderResourceView> m_irradianceSRV; // Diffuse (부드럽게 전처리한 Texture)
		ComPtr<ID3D11ShaderResourceView> m_brdfSRV;
	};
}
