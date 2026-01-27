#pragma once
#include "EffectActor.h"

namespace DE {
	class SubEmitterEffectsActor : public EffectActor
	{
		using Super = EffectActor;
	public:
		SubEmitterEffectsActor(const std::wstring& name);
		virtual ~SubEmitterEffectsActor() override;

		void Update(const float& deltaTime) override;

		bool NeedsExternalPreset() const override { return false; }
	};
}