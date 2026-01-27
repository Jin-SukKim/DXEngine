#pragma once
#include "Actor.h"

namespace DE {
	class ModelComponent;
	class ParticleSystem;

	class RoseEffect : public Actor
	{
		using Super = Actor;
	public:
		RoseEffect(const std::wstring& name);
		virtual ~RoseEffect() override;

		void Initialize() override;
		void Update(const float& deltaTime) override;
		void Render() override;
	private:
		ModelComponent* m_model = nullptr;
		ParticleSystem* m_orbit = nullptr;

		int count = 0;
	};
}