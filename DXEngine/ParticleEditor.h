#pragma once
#include "Scene.h"

namespace DE {

	class ParticleEmitter;
	class SquareActor;
	class EffectActor;
	class ParticleSpawner;
	class Firework;
	class RoseEffect;
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
	SquareActor* ground;

	SampleActor* m_sample;
	ParticleSpawner* m_spanwer;
	Firework* m_firework;
	RoseEffect* m_rose;

	InputAction m_click;
};


}

