#pragma once
#include "Scene.h"
#include <vector> // �߰�

namespace DE {

	class ParticleEmitter;
	class SquareActor;
	class EffectActor;
	class ParticleSpawner;
	class Firework;
	class RoseEffect;
	class TestActor;
	class ParticleSystem; // ���� ���� Ȯ��
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

		ParticleSystem* m_test1 = nullptr; // ���� ����Ʈ��
		ParticleSystem* m_test2 = nullptr; // ���ڱ� �߰��Ǵ� ����Ʈ��
		ParticleSystem* m_test3 = nullptr;
		TestActor* m_testActor = nullptr;
		std::vector<FireEffect*> m_fireTests;

		EffectActor* m_metalSpark = nullptr;
		EffectActor* m_sparkBurst = nullptr;
		EffectActor* m_ember = nullptr;
		EffectActor* m_curlSmoke = nullptr;
		EffectActor* m_curlFirefly = nullptr;
		EffectActor* m_curlMagicAura = nullptr;
		EffectActor* m_curlSpiritWisp = nullptr;
		EffectActor* m_test = nullptr;
		EffectActor* m_swordClash = nullptr;

		// Phase 2 effects
		EffectActor* m_rain = nullptr;
		EffectActor* m_snow = nullptr;
		EffectActor* m_tornado = nullptr;
		EffectActor* m_solarFlare = nullptr;
		EffectActor* m_crystalShatter = nullptr;
		EffectActor* m_bubble = nullptr;
		EffectActor* m_firefly = nullptr;
		EffectActor* m_portalGateway = nullptr;
		EffectActor* m_healAura = nullptr;
		EffectActor* m_shieldBarrier = nullptr;
		EffectActor* m_levelUp = nullptr;
		EffectActor* m_teleport = nullptr;
		EffectActor* m_deathDissolve = nullptr;

	};
}