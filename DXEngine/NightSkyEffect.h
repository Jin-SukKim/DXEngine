#pragma once
#include "EffectActor.h"

namespace DE {
	class NightSkyEffect : public EffectActor
	{
		using Super = EffectActor;
	public:
		NightSkyEffect(const std::wstring& name);
		virtual ~NightSkyEffect() override;

		bool NeedsExternalPreset() const override { return false; }
	};
}