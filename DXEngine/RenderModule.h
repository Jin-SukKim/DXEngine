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
	void Initialize(ParticleInitContext& ctx) override;
	void OnSpawn(SimulationContext& ctx) override;
	virtual void Draw(const RenderContext& ctx);
	void SetBlendState();
	ModulePriority GetPriority() override { return ModulePriority::Render; }
	void LoadFromJson(const json& data) override;
public:
	BlendMode blendMode = BlendMode::Additive;
protected:
	BitonicSort m_sort;
	ComputeShader m_InitSortKeysCS;
	ID3D11BlendState* m_blendState = NULL;
};

class BillboardRenderModule : public RenderModule
{
public:
	void Draw(const RenderContext& context) override;
private:
	// TODO: Texture Ãß°¡
};
}

