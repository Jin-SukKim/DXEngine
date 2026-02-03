#pragma once
#include "Scene.h"

namespace DE {

	class ParticleEmitter;
	class SquareActor;
	class EffectActor;
	class ParticleSpawner;
	class Firework;
	class RoseEffect;
	class TestActor;

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

private:
	void RunMemoryPoolTest(const float& dt);
	
private:
	SquareActor* ground = nullptr;

	SampleActor* m_sample = nullptr;
	ParticleSpawner* m_spanwer = nullptr;
	Firework* m_firework = nullptr;
	RoseEffect* m_rose = nullptr;

	InputAction m_click;

	ParticleSystem* m_test1 = nullptr;
	ParticleSystem* m_test2 = nullptr;
	TestActor* m_testActor = nullptr;

	// 메모리 풀 테스트용
	std::vector<ParticleSystem*> m_testSystems;
	float m_testTimer = 0.0f;
	int m_testPhase = 0;
	bool m_runTest = false;
	bool m_useRandomEffect = true;
};

}

