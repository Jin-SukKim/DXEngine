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
	SquareActor* ground = nullptr;

	SampleActor* m_sample = nullptr;
	ParticleSpawner* m_spanwer = nullptr;
	Firework* m_firework = nullptr;
	RoseEffect* m_rose = nullptr;

	InputAction m_click;

	ParticleSystem* m_test1 = nullptr;
	ParticleSystem* m_test2 = nullptr;
	TestActor* m_testActor = nullptr;
};


}

