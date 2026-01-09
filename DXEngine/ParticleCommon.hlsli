#ifndef __PARTICLE_COMMON_HLSLI__
#define __PARTICLE_COMMON_HLSLI__

struct Particle
{
	float3 position;
	float3 velocity;
	float3 color;
	float life;
	float lifeMax;
	float size;
	float rotation;
	float rotSpeed;
};

cbuffer ParticleConsts : register(b0)
{
	float dt;              // 델타 타임
	float time;            // 경과 시간 (랜덤 시드용)
	uint spawnCount;       // 이번 프레임에 생성할 개수
	uint maxParticles;     // 최대 파티클 수

	// Spawn
	float3 spawnVolume; // 생성 범위
	float lifeTimeBase; // 기본 수명

	float lifeTimeRand; // 랜덤 추가 생명
	float3 velocityBase; // 기본 방향 속도

	float3 velocityRand; // 랜덤 추가 방향 및 속도
	float velocity; // 속도

	// Physics
	float3 gravity; // 중력 or 지속적으로 작용하는 힘
	float drag; // 공기저항

	// Visual
	float2 minMaxSize;
	float2 minMaxRotateSpeed;

	float3 startColor;
	float padding1;

	float3 endColor;
	float padding2;
};

#endif // __PARTICLE_COMMON_HLSLI__