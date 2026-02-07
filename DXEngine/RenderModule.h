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
	virtual int GetTextureMode() const { return -1; }
	virtual int GetSingleTextureIdx() const { return -1; }
	virtual bool IsMesh() const { return false; }
	virtual bool IsBatchable() const { return false; }
	std::unique_ptr<ParticleModule> Clone() const override = 0;
	void CopyBasicSettings(RenderModule* cloned) const;
public:
	BlendMode blendMode = BlendMode::Additive;
protected:
	ID3D11BlendState* m_blendState = NULL;
	// BitonicSort ���� - ParticleEmitter�� ����
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

	int GetTextureMode() const override { return static_cast<int>(m_textureMode); }
	int GetSingleTextureIdx() const override { return m_singleTextureIdx; }
	bool IsMesh() const override { return false; }
	bool IsBatchable() const override {
		// TextureArray 모드이거나, 같은 SingleTexture를 공유하는 경우 배칭 가능
		// Material 모드는 per-emitter 머티리얼 바인딩이 필요하므로 배칭 불가
		return m_textureMode != BillboardTextureMode::Material;
	}
private:
	// 0 : TextureArray (Size�� �����Ǿ� ����)
	// 1 : Single Texture (���� Texture 1��, �پ��� �ػ� ����)
	// 2 : MaterialModule�� ��� (PBR)
	BillboardTextureMode m_textureMode = BillboardTextureMode::TextureArray;
	// Texture ����
	std::string m_texturePath;
	int m_textureIdx = -1;

	// �پ��� �ػ��� Texture 1���� ����Ҷ�
	int m_singleTextureIdx = -1;

	Vector2 m_frameTiles = { 1, 1 };
	UINT m_frameCount = 1;
	// IndirectArgsBuffer ���� - ParticleEmitter�� ����
};

class MeshRenderModule : public RenderModule
{
public:
	void Initialize(ParticleInitContext& ctx) override;
	void OnRender(const RenderContext& ctx) override;
	void LoadFromJson(const json& data) override;
	int GetModelIndex() const override { return m_modelIdx; }
	int GetTextureMode() const override { return -1; } // Mesh는 항상 Material
	int GetSingleTextureIdx() const override { return -1; }
	bool IsMesh() const override { return true; }
	bool IsBatchable() const override { return true; } // 같은 모델+BlendMode면 배칭 가능
	std::unique_ptr<ParticleModule> Clone() const override;
private:
	int m_modelIdx = -1;
	UINT m_meshCount = 0;
	// IndirectArgsBuffer ���� - ParticleEmitter�� ����
};
}

