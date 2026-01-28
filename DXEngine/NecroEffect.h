#pragma once
#include "EffectActor.h"

namespace DE {
	class NecroEffect : public EffectActor
	{
		using Super = EffectActor;
	public:
		NecroEffect(const std::wstring& name);
		virtual ~NecroEffect() override;

		bool NeedsExternalPreset() const override { return false; }
	};
}