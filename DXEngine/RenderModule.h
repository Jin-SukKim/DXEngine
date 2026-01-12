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
	void SetBlendState();
	ModulePriority GetPriority() override { return ModulePriority::Render; }
public:
	BlendMode blendMode = BlendMode::Additive;
private:
	ID3D11Device* m_device;
};

}

