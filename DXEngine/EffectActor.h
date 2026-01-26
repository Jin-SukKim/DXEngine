#pragma once
#include "Actor.h"

namespace DE {
	class ParticleSystem;

	class EffectActor : public Actor
	{
		using Super = Actor;
	public:
		EffectActor(const std::wstring& name);
		virtual ~EffectActor() override {}

		void Initialize() override;
		void Update(const float& deltaTime) override;
	private:
		ParticleSystem* m_particles;
	};
}