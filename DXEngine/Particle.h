#pragma once
#include "ParticleProperties.h"

namespace DE {
	struct Particle {
		Vector3 position = Vector3(0.f, 0.f, 0.f);
		Vector3 velocity = Vector3(0.f, 0.f, 0.f);
		Vector3 color = Vector3(0.f, 0.f, 0.f);
		float life = -1.f;
		float lifeMax = 0.f;
		float size = 1.f;
	};

	struct ParticleConsts {
		float dt = 0.f;              // 델타 타임
		float time = 0.f;            // 경과 시간 (랜덤 시드용)
		UINT spawnCount = 0;       // 이번 프레임에 생성할 개수
		UINT maxParticles = 0;     // 최대 파티클 수

		// Spawn
		Vector3 spawnVolume = Vector3(0.f, 0.f, 0.f); // 생성 범위
		float padding1;
		Vector2 lifeRange = Vector2(1.f, 1.f);
		Vector2 padding2;

		// Force
		Vector3 velocity = Vector3(0.f);
		float padding3;
		Vector2 speedRange = Vector2(0.f);
		Vector2 padding4;

		Vector3 randomDir = Vector3(0.f);
		float padding5;
		Vector3 gravity = Vector3(0.f, 0.f, 0.f); // 중력 or 지속적으로 작용하는 힘
		float drag = 0.f; // 공기저항

		// Visual
		Vector2 sizeRange= Vector2(1.0f, 1.0f);
		Vector2 padding6;
		Vector4 startColor = Vector4(1.f, 1.f, 1.f, 1.f);
		Vector4 endColor = Vector4(1.f, 1.f, 1.f, 1.f);
	};
}