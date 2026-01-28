#pragma once
#include "EffectActor.h"

namespace DE {
	class BillboardEffectsActor : public EffectActor
	{
		using Super = EffectActor;
	public:
		BillboardEffectsActor(const std::wstring& name);
		virtual ~BillboardEffectsActor() override;

		bool NeedsExternalPreset() const override { return false; }
	private:
		ParticleSystem* m_sprite;
		ParticleSystem* m_textures;
	};
}