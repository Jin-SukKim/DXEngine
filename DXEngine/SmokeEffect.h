#pragma once
#include "EffectActor.h"

namespace DE {
	class SmokeEffect : public EffectActor
	{
		using Super = EffectActor;
	public:
		SmokeEffect(const std::wstring& name);
		virtual ~SmokeEffect() override;

		bool NeedsExternalPreset() const override { return false; }
	};
}