#pragma once
#include "EffectActor.h"

namespace DE {
	class SwordClashEffect : public EffectActor
	{
		using Super = EffectActor;
	public:
		SwordClashEffect(const std::wstring& name);
		virtual ~SwordClashEffect() override;

		bool NeedsExternalPreset() const override { return false; }
	};
}