#ifndef __PARTICLE_COMMON_HLSLI__
#define __PARTICLE_COMMON_HLSLI__

struct Particle
{
    float3 position;
    float3 velocity;
    float3 color;
    float life;
    float size;
};


cbuffer ParticleConsts : register(b0)
{
    float dt;
    float time;
    float spawnRate; // 초당 생성률
    uint maxParticles; // 최대 파티클 수
};


#endif // __PARTICLE_COMMON_HLSLI__