#pragma once
#include "Scene.h"
#include <vector> // 추가

namespace DE {

	class ParticleEmitter;
	class SquareActor;
	class EffectActor;
	class ParticleSpawner;
	class Firework;
	class RoseEffect;
	class TestActor;
	class ParticleSystem; // 전방 선언 확인
	class SmokeActor;
	class FireEffect;

	class ParticleEditor : public Scene
	{
	public:
		ParticleEditor();
		~ParticleEditor();

		void Initialize() override;
		void Update(const float& deltaTime) override;
		void StressTest(const float& dt);
		void UpdateGUI() override;
		void Render() override;

		void ClickEvent();
		void ClickDestroy();
	private:
		SquareActor* ground = nullptr;

		SampleActor* m_sample = nullptr;
		ParticleSpawner* m_spanwer = nullptr;
		ParticleSpawner* m_spanwer2 = nullptr;
		Firework* m_firework = nullptr;
		RoseEffect* m_rose = nullptr;
		SmokeActor* m_smoke = nullptr;

		InputAction m_Lclick;
		InputAction m_Rclick;

		ParticleSystem* m_test1 = nullptr; // 지속 이펙트용
		ParticleSystem* m_test2 = nullptr; // 갑자기 추가되는 이펙트용
		ParticleSystem* m_test3 = nullptr;
		TestActor* m_testActor = nullptr;
		std::vector<FireEffect*> m_fireTests;
		// [신규] 스트레스 테스트용 변수
		float m_stressTime = 0.0f;
		std::vector<ParticleSystem*> m_stressSystems; // 동적으로 마구 생성된 녀석들 관리
	};
}