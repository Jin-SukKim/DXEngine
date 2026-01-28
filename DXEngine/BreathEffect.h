#pragma once
#include "EffectActor.h"
namespace DE {
	class BreathEffect : public EffectActor
	{
		using Super = EffectActor;
	public:
		BreathEffect(const std::wstring& name);
		virtual ~BreathEffect() override;

		bool NeedsExternalPreset() const override { return false; }
	};
}