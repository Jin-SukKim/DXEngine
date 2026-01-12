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
		float lifeTimeBase = 0.f; // 기본 수명

		float lifeTimeRand = 0.f; // 랜덤 추가 생명
		Vector3 velocityBase = Vector3(0.f, 0.f, 0.f); // 기본 방향 속도

		Vector3 velocityRand = Vector3(0.f, 0.f, 0.f); // 랜덤 추가 방향 및 속도
		float velocity = 0.f; // 속도

		// Physics
		Vector3 gravity = Vector3(0.f, 0.f, 0.f); // 중력 or 지속적으로 작용하는 힘
		float drag = 0.f; // 공기저항

		// Visual
		Vector2 minMaxSize = Vector2(0.f, 0.f);
		Vector2 minMaxRotateSpeed;

		Vector3 startColor = Vector3(0.f, 0.f, 0.f);
		float padding1 = 0.f;

		Vector3 endColor = Vector3(0.f, 0.f, 0.f);
		float padding2 = 0.f;
	};
}