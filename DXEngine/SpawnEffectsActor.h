#pragma once
#include "EffectActor.h"

namespace DE {
class SpawnEffectsActor : public EffectActor
{
	using Super = EffectActor;
public:
	SpawnEffectsActor(const std::wstring& name);
	virtual ~SpawnEffectsActor() override;

	void Initialize() override;
	void Update(const float& deltaTime) override;

	bool NeedsExternalPreset() const override { return false; }
private:
	ParticleSystem* m_burst;
	ParticleSystem* m_custom;
	ParticleSystem* m_hollow;
	ParticleSystem* m_sphere;
	ParticleSystem* m_texture;
};
}

