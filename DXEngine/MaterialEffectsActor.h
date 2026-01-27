#pragma once
#include "EffectActor.h"

namespace DE {
	class MaterialEffectsActor : public EffectActor
	{
		using Super = EffectActor;
	public:
		MaterialEffectsActor(const std::wstring& name);
		virtual ~MaterialEffectsActor() override;

		void Update(const float& deltaTime) override;

		bool NeedsExternalPreset() const override { return false; }
	};
}