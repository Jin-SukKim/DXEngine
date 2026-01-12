#pragma once
#include "ParticleModule.h"
#include "BitonicSort.h"

namespace DE {
enum class BlendMode {
	Additive,
	AlphaBlend,
	Opaque,
};

class RenderModule : public ParticleModule
{
public:
	void Initialize(ID3D11Device* device, ParticleEmitter* config) override;
	void OnSpawn(ID3D11DeviceContext* context) override;
	virtual void Draw(ID3D11DeviceContext* context,
		ID3D11Buffer* indirectArgs,
		ID3D11ShaderResourceView* particleSRV,
		ID3D11ShaderResourceView* sortSRV);
	void SetBlendState();
	ModulePriority GetPriority() override { return ModulePriority::Render; }
	void LoadFromJson(const json& data) override;
public:
	BlendMode blendMode = BlendMode::Additive;
protected:
	ID3D11Device* m_device;
	BitonicSort m_sort;
	ComputeShader m_InitSortKeysCS;
};

class BillboardRenderModule : public RenderModule
{
public:
	void Draw(ID3D11DeviceContext* context,
		ID3D11Buffer* indirectArgs,
		ID3D11ShaderResourceView* particleSRV,
		ID3D11ShaderResourceView* m_countSRV) override;
private:
	// TODO: Texture Ãß°¡
};
}

