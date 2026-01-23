#pragma once
#include "Actor.h"

namespace DE {
	class ModelComponent;
	class BoundComponent;
	class ParticleSystemComponent;

	class SampleActor : public Actor
	{
		using Super = Actor;
	public:
		SampleActor(const std::wstring& name);
		virtual ~SampleActor() override {}

		void Initialize() override;
		void Update(const float& deltaTime) override;
		void Render() override;
	private:
		ModelComponent* m_sample;
		BoundComponent* m_boundVolume;
		ParticleSystemComponent* m_particles;
	};
}