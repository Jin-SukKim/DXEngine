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
	void UpdateArgs(const SimulationContext& ctx) override;
	void OnUpdate(const SimulationContext& ctx) override;
	virtual void OnRender(const RenderContext& ctx) override;
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
	void OnSpawn(SimulationContext& ctx) override;
	void UpdateArgs(const SimulationContext& ctx) override;
	void OnRender(const RenderContext& ctx) override;
	void LoadFromJson(const json& data) override;
private:
	// Texture 包府
	std::string m_texturePath;
	int m_textureIdx = -1;
	Vector2 m_frameTiles = { 1, 1 };
	UINT m_frameCount = 1;
	IndirectArgsBuffer<DrawInstancedArgs> m_argsBuffer;
};

class MeshRenderModule : public RenderModule
{
public:
	void Initialize(ParticleInitContext& ctx) override;
	void OnSpawn(SimulationContext& ctx) override;
	void UpdateArgs(const SimulationContext& ctx) override;
	void OnRender(const RenderContext& ctx) override;
	void LoadFromJson(const json& data) override;
private:
	// Texture 包府
	int m_modelIdx = -1;

	IndirectArgsBuffer<DrawIndexedInstancedArgs>  m_meshArgs;
	ComputeShader m_argsUpdateCS;
	UINT m_meshCount = 0;
};
}

