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
		float dt;              // 델타 타임
		float time;            // 경과 시간 (랜덤 시드용)
		float spawnCount;       // 이번 프레임에 생성할 개수
		UINT maxParticles;     // 최대 파티클 수
	};
}