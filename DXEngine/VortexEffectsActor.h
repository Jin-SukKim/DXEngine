#pragma once
#include "EffectActor.h"
namespace DE {
	class VortexEffectsActor : public EffectActor
	{
		using Super = EffectActor;
	public:
		VortexEffectsActor(const std::wstring& name);
		virtual ~VortexEffectsActor() override;

		void Update(const float& deltaTime) override;

		bool NeedsExternalPreset() const override { return false; }
	private:
		ParticleSystem* m_tornado;
	};
}