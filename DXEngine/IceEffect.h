#pragma once
#include "EffectActor.h"

namespace DE {
	class IceEffect : public EffectActor
	{
		using Super = EffectActor;
	public:
		IceEffect(const std::wstring& name);
		virtual ~IceEffect() override;

		bool NeedsExternalPreset() const override { return false; }
	};
}