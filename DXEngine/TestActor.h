#pragma once
#include "EffectActor.h"

namespace DE {
	class ParticleSystem;

	class TestActor : public EffectActor
	{
		using Super = EffectActor;
	public:
		TestActor(const std::wstring& name);
		virtual ~TestActor() override;
		bool NeedsExternalPreset() const override { return false; }
	private:
	};
}