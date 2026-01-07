#pragma once
#include "Scene.h"

namespace DE {

	class ParticleEmitter;

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
	std::unique_ptr<ParticleEmitter> particleEmitter;
};


}

