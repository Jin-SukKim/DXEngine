#pragma once
#include "EffectActor.h"

namespace DE {
	class PortalEffect : public EffectActor
	{
		using Super = EffectActor;
	public:
		PortalEffect(const std::wstring& name);
		virtual ~PortalEffect() override;

		bool NeedsExternalPreset() const override { return false; }
	};
}