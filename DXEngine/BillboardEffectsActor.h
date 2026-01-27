#pragma once
#include "EffectActor.h"

namespace DE {
	class BillboardEffectsActor : public EffectActor
	{
		using Super = EffectActor;
	public:
		BillboardEffectsActor(const std::wstring& name);
		virtual ~BillboardEffectsActor() override;

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