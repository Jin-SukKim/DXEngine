#pragma once
#include "EffectActor.h"
namespace DE {
	class VisualEffectsActor : public EffectActor
	{
		using Super = EffectActor;
	public:
		VisualEffectsActor(const std::wstring& name);
		virtual ~VisualEffectsActor() override;

		void Update(const float& deltaTime) override;

		bool NeedsExternalPreset() const override { return false; }
	private:
		ParticleSystem* m_rotation;
	};
}