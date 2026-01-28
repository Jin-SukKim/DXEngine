#pragma once
#include "EffectActor.h"

namespace DE {
	class PhoenixEffect : public EffectActor
	{
		using Super = EffectActor;
	public:
		PhoenixEffect(const std::wstring& name);
		virtual ~PhoenixEffect() override;

		bool NeedsExternalPreset() const override { return false; }
	};
}