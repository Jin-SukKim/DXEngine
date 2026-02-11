#pragma once
#include "EffectActor.h"

namespace DE {
	class FireEffect : public EffectActor
	{
		using Super = EffectActor;
	public:
		FireEffect(const std::wstring& name);
		virtual ~FireEffect() override;

		bool NeedsExternalPreset() const override { return false; }
	};
}