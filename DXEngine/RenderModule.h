#pragma once
#include "ParticleModule.h"

namespace DE {
enum class BlendMode {
	Opaque,      // 0 - 먼저 렌더링
	Additive,    // 1 - 중간 렌더링
	AlphaBlend,  // 2 - 마지막 렌더링
};

class RenderModule : public ParticleModule
{
public:
	void Initialize(ParticleInitContext& ctx) override;
	void OnSpawn(SimulationContext& ctx) override;
	void SetBlendState();
	ModulePriority GetPriority() override { return ModulePriority::Render; }
	void LoadFromJson(const json& data) override;
	virtual int GetModelIndex() const { return -1; };
	virtual std::unique_ptr<ParticleModule> Clone() const override = 0;
	void CopyBasicSettings(RenderModule* cloned) const;
	int GetModelIndex() { return m_modelIdx; }
public:
	BlendMode blendMode = BlendMode::Additive;
protected:
	int m_modelIdx = -1;
	ID3D11BlendState* m_blendState = NULL;
	// BitonicSort  - ParticleEmitter 
};

class BillboardRenderModule : public RenderModule
{
public:
	void Initialize(ParticleInitContext& ctx) override;
	void LoadFromJson(const json& data) override;
	std::unique_ptr<ParticleModule> Clone() const override;
private:
	float m_softDistance = 0.0f;
	float m_velocityStretchFactor = 0.0f;
	// All texture handling moved to MaterialModule
	// IndirectArgsBuffer  - ParticleEmitter

	UINT m_uvDistortEnabled = true;
	float m_uvDistortFrequency = 1.0f;
	float m_uvDistortStrength = 0.01f;
	Vector2 m_uvDistortScroll = Vector2(0.f, 0.5f);
};

class MeshRenderModule : public RenderModule
{
public:
	void Initialize(ParticleInitContext& ctx) override;
	void LoadFromJson(const json& data) override;
	int GetModelIndex() const override { return m_modelIdx; }
	std::unique_ptr<ParticleModule> Clone() const override;
private:
	UINT m_meshCount = 0;
	// IndirectArgsBuffer  - ParticleEmitter 
};
}

