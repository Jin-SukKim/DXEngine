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
	void UpdateArgs(const RenderContext& ctx) override;
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
	// BitonicSort 제거 - ParticleEmitter가 소유
};

class BillboardRenderModule : public RenderModule
{
	enum class BillboardTextureMode {
		Material, SingleTexture, TextureArray
	};
public:
	void OnSpawn(SimulationContext& ctx) override;
	void UpdateArgs(const RenderContext& ctx) override;
	void OnRender(const RenderContext& ctx) override;
	void LoadFromJson(const json& data) override;
	std::unique_ptr<ParticleModule> Clone() const override;
private:
	// 0 : TextureArray (Size가 고정되어 있음)
	// 1 : Single Texture (개별 Texture 1개, 다양한 해상도 가능)
	// 2 : MaterialModule을 사용 (PBR)
	BillboardTextureMode m_textureMode = BillboardTextureMode::TextureArray;
	// Texture 관리
	std::string m_texturePath;
	int m_textureIdx = -1;

	// 다양한 해상도의 Texture 1개만 사용할때
	int m_singleTextureIdx = -1;

	Vector2 m_frameTiles = { 1, 1 };
	UINT m_frameCount = 1;
	// IndirectArgsBuffer 제거 - ParticleEmitter가 소유
};

class MeshRenderModule : public RenderModule
{
public:
	void Initialize(ParticleInitContext& ctx) override;
	void OnSpawn(SimulationContext& ctx) override;
	void UpdateArgs(const RenderContext& ctx) override;
	void OnRender(const RenderContext& ctx) override;
	void LoadFromJson(const json& data) override;
	int GetModelIndex() const override { return m_modelIdx; }
	std::unique_ptr<ParticleModule> Clone() const override;
private:
	int m_modelIdx = -1;
	UINT m_meshCount = 0;
	// IndirectArgsBuffer 제거 - ParticleEmitter가 소유
};
}

