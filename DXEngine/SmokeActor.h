#pragma once
#include "EffectActor.h"

namespace DE {
	class SmokeActor : public EffectActor
	{
		using Super = EffectActor;
	public:
		SmokeActor(const std::wstring& name);
		virtual ~SmokeActor() override;

		bool NeedsExternalPreset() const override { return false; }
	};
}