#pragma once
#include "Scene.h"

namespace DE {
	class SquareActor;
	class ParticleSpawner;
	class SpawnEffectsActor;
	class SnowActor;
	class BasicParticleScene : public Scene
	{
	public:
		BasicParticleScene();
		~BasicParticleScene();

		void Initialize() override;
		void Update(const float& deltaTime) override;

		void ClickEvent();
	private:
		SquareActor* ground;
		InputAction m_click;

		ParticleSpawner* m_spanwer;

		SpawnEffectsActor* m_spawnModule;
		SnowActor* m_snow;
	};


}

