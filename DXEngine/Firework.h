#pragma once
#include "Actor.h"

namespace DE {
	class ParticleSystem;

	class Firework : public Actor
	{
		using Super = Actor;
	public:
		Firework(const std::wstring& name);
		virtual ~Firework() override {}

		void Initialize() override;
		void Update(const float& deltaTime) override;
	private:
		ParticleSystem* m_up;
		ParticleSystem* m_firework;

		int count = 0;
	};
}