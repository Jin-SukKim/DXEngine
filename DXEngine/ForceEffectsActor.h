#pragma once
#include "EffectActor.h"

namespace DE {
	class ForceEffectsActor : public EffectActor
	{
		using Super = EffectActor;
	public:
		ForceEffectsActor(const std::wstring& name);
		virtual ~ForceEffectsActor() override;

		void Update(const float& deltaTime) override;

		bool NeedsExternalPreset() const override { return false; }
	private:
		ParticleSystem* m_fountain;
		ParticleSystem* m_gravity;
	};
}