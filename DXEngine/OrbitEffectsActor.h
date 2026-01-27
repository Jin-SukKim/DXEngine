#pragma once
#include "EffectActor.h"
namespace DE {
	class OrbitEffectsActor : public EffectActor
	{
		using Super = EffectActor;
	public:
		OrbitEffectsActor(const std::wstring& name);
		virtual ~OrbitEffectsActor() override;

		void Update(const float& deltaTime) override;

		bool NeedsExternalPreset() const override { return false; }
	private:
		ParticleSystem* m_fastTrail;
		ParticleSystem* m_spiral;
		ParticleSystem* m_tilted;
		ParticleSystem* m_orbitX;
	};
}