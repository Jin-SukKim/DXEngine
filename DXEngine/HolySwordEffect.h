#pragma once
#include "EffectActor.h"

namespace DE {
	class HolySwordEffect : public EffectActor
	{
		using Super = EffectActor;
	public:
		HolySwordEffect(const std::wstring& name);
		virtual ~HolySwordEffect() override;

		bool NeedsExternalPreset() const override { return false; }
	};
}