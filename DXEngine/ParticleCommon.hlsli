#ifndef __PARTICLE_COMMON_HLSLI__
#define __PARTICLE_COMMON_HLSLI__

cbuffer ParticleConsts : register(b0)
{
	float dt;            // 델타 타임
	float time;            // 경과 시간 (랜덤 시드용)
	uint spawnCount;       // 이번 프레임에 생성할 개수
	uint maxParticles;     // 최대 파티클 수

	// Spawn
	float3 spawnVolume;  // 생성 범위
	float padding1;
	float2 lifeRange;
	float2 padding2;

	// Force
	float3 velocity;
	float padding3;
	float2 speedRange;
	float2 padding4;

	float3 randomDir;
	float padding5;
	float3 gravity; // 중력 or 지속적으로 작용하는 힘
	float drag; // 공기저항

	// Visual
	float2 sizeRange;
	float2 padding6;
	float4 startColor;
	float4 endColor;

	float3 minRotation;
	float padding7;
	float3 maxRotation;
	float padding8;
	float3 minRotSpeed;
	float padding9;
	float3 maxRotSpeed;
	float padding10;
};

struct SortElement
{
    float key;
    uint value;
};

#endif // __PARTICLE_COMMON_HLSLI__