#pragma once
#include "Actor.h"

namespace DE {
	class ModelComponent;
	class ParticleSystem;

	class SampleActor : public Actor
	{
		using Super = Actor;
	public:
		SampleActor(const std::wstring& name);
		virtual ~SampleActor() override;  // 소멸자 구현 필요

		void Initialize() override;
		void Update(const float& deltaTime) override;
		void Render() override;
	private:
		ModelComponent* m_sample;
		ParticleSystem* m_particles = nullptr;  // nullptr 초기화
	};
}