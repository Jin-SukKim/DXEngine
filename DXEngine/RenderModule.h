#pragma once
#include "ParticleModule.h"

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
	virtual void OnRender(const RenderContext& ctx) override;
	void SetBlendState();
	ModulePriority GetPriority() override { return ModulePriority::Render; }
	void LoadFromJson(const json& data) override;
	virtual int GetModelIndex() const { return -1; };
	virtual std::unique_ptr<ParticleModule> Clone() const override = 0;
	void CopyBasicSettings(RenderModule* cloned) const;

public:
	BlendMode blendMode = BlendMode::Additive;
protected:
	ID3D11BlendState* m_blendState = NULL;
	// BitonicSort  - ParticleEmitter 
};

class BillboardRenderModule : public RenderModule
{
	enum class BillboardTextureMode {
		Material, SingleTexture, TextureArray
	};
public:
	void Initialize(ParticleInitContext& ctx) override;
	void OnRender(const RenderContext& ctx) override;
	void LoadFromJson(const json& data) override;
	std::unique_ptr<ParticleModule> Clone() const override;
private:
	// 0 : TextureArray (Size)
	// 1 : Single Texture ( Texture 1 )
	// 2 : MaterialModule  (PBR)
	BillboardTextureMode m_textureMode = BillboardTextureMode::TextureArray;
	// Texture 
	std::string m_texturePath;
	int m_textureIdx = -1;

	// Texture
	int m_singleTextureIdx = -1;

	Vector2 m_frameTiles = { 1, 1 };
	UINT m_frameCount = 1;
	// IndirectArgsBuffer  - ParticleEmitter 
};

class MeshRenderModule : public RenderModule
{
public:
	void Initialize(ParticleInitContext& ctx) override;
	void OnRender(const RenderContext& ctx) override;
	void LoadFromJson(const json& data) override;
	int GetModelIndex() const override { return m_modelIdx; }
	std::unique_ptr<ParticleModule> Clone() const override;
private:
	int m_modelIdx = -1;
	UINT m_meshCount = 0;
	// IndirectArgsBuffer  - ParticleEmitter 
};
}

