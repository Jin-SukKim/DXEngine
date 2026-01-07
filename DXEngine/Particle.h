#pragma once
#include "ParticleProperties.h"

namespace DE {
	struct Particle {
		Vector3 position;
		Vector3 velocity;
		Vector3 color;
		float life = -1.f;
		float size = 1.f;
	};

	struct ParticleConsts {
		float dt;
		Vector3 dummy;
	};
}

