#pragma once
#include "EffectActor.h"

namespace DE {
	class ParticleSystem;

	class RoseEffect : public EffectActor
	{
		using Super = EffectActor;
	public:
		RoseEffect(const std::wstring& name);
		virtual ~RoseEffect() override;

		void Initialize() override;
		void Update(const float& deltaTime) override;
		bool IsFinished() const override;
		bool NeedsExternalPreset() const override { return false; }
	private:
		ParticleSystem* m_orbit;

		int count = 0;
	};
}