#pragma once
#include "EffectActor.h"

namespace DE {
	class TsunamiEffect : public EffectActor
	{
		using Super = EffectActor;
	public:
		TsunamiEffect(const std::wstring& name);
		virtual ~TsunamiEffect() override;

		bool NeedsExternalPreset() const override { return false; }
	};
}