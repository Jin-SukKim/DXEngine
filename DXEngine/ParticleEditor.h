#pragma once
#include "Scene.h"
#include <vector>

namespace DE {

	class ParticleEmitter;
	class SquareActor;
	class EffectActor;
	class ParticleSpawner;
	class Firework;
	class RoseEffect;
	class TestActor;
	class ParticleSystem; 
	class SmokeActor;
	class FireEffect;

	class ParticleEditor : public Scene
	{
	public:
		ParticleEditor();
		~ParticleEditor();

		void Initialize() override;
		void Update(const float& deltaTime) override;
		void UpdateGUI() override;
		void Render() override;

		void ClickEvent();
		void ClickDestroy();
	private:
		SquareActor* ground = nullptr;

		SampleActor* m_sample = nullptr;
		ParticleSpawner* m_spawner = nullptr;
		ParticleSpawner* m_spawner2 = nullptr;
		Firework* m_firework = nullptr;
		RoseEffect* m_rose = nullptr;
		SmokeActor* m_smoke = nullptr;

		InputAction m_Lclick;
		InputAction m_Rclick;

		// Spectacular 
		EffectActor* m_arcaneCircle = nullptr;
		EffectActor* m_crystalShatter = nullptr;
		EffectActor* m_galaxySwirl = nullptr;

		// UnrealQuality 
		EffectActor* m_portalGateway = nullptr;

		// Explosion
		EffectActor* m_explosion = nullptr;

		// Combination
		EffectActor* m_holySword = nullptr;
		EffectActor* m_swordClash = nullptr;

		// Magic
		EffectActor* m_magicCast = nullptr;

		// ForceModule
		EffectActor* m_curlNoiseFirefly = nullptr;

		// Misc / Custom
		EffectActor* m_fireEffect = nullptr;
		EffectActor* m_sparkBurst = nullptr;
		EffectActor* m_fog = nullptr;
		EffectActor* m_custom = nullptr;

		// 추가
		EffectActor* m_boxMesh = nullptr;
		EffectActor* m_portal = nullptr;
		EffectActor* m_orbit = nullptr;

		std::vector<EffectActor*> m_clickExplosions;

		// Realistic
		EffectActor* m_rain = nullptr;

		// Legacy test handles (kept for destructor cleanup)
		ParticleSystem* m_test1 = nullptr;
		ParticleSystem* m_test2 = nullptr;
		ParticleSystem* m_test3 = nullptr;

		TestActor* m_testActor = nullptr;
		std::vector<FireEffect*> m_fireTests;

	};
}