#pragma once
#include "EffectActor.h"

namespace DE {
	class StarEffect : public EffectActor
	{
		using Super = EffectActor;
	public:
		StarEffect(const std::wstring& name);
		virtual ~StarEffect() override;

		bool NeedsExternalPreset() const override { return false; }
	};
}