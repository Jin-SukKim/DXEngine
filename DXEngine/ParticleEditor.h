#pragma once
#include "Scene.h"

namespace DE {
	class SquareActor;
	class PointLight;

class ParticleEditor : public Scene
{
public:
	ParticleEditor();
	~ParticleEditor();

	void Initialize() override;
	void Update(const float& deltaTime) override;
	void UpdateGUI() override;
	void Render() override;

private:
	SquareActor* m_ground;
	PointLight* m_pointLight;
};


}

