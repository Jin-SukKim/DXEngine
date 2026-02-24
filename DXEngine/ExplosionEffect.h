#pragma once
#include "EffectActor.h"

namespace DE {
	class ExplosionEffect : public EffectActor
	{
		using Super = EffectActor;
	public:
		ExplosionEffect(const std::wstring& name);
		virtual ~ExplosionEffect() override;

		bool NeedsExternalPreset() const override { return false; }
	};
}