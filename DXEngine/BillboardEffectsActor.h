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
		ParticleSystem* m_single;
		ParticleSystem* m_sprite;
		ParticleSystem* m_textures;
	};
}