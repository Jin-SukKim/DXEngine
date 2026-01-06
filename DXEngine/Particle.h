#pragma once
#include "ParticleProperties.h"

namespace DE {
class Particle
{
public:
	void Initialize();
	void Update();
	void Render();
private:
	ParticleProperties properties;
};

}

