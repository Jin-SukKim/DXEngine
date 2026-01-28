#pragma once
#include "EffectActor.h"
namespace DE {
	class MagicEffect : public EffectActor
	{
		using Super = EffectActor;
	public:
		MagicEffect(const std::wstring& name);
		virtual ~MagicEffect() override;

		bool NeedsExternalPreset() const override { return false; }
	};
}