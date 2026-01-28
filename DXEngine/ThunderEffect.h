#pragma once
#include "EffectActor.h"

namespace DE {
	class ThunderEffect : public EffectActor
	{
		using Super = EffectActor;
	public:
		ThunderEffect(const std::wstring& name);
		virtual ~ThunderEffect() override;

		bool NeedsExternalPreset() const override { return false; }
	};
}