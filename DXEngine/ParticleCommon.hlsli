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
    uint maxParticles; // 최대 파티클 수
    uint spawnRate; // 매 프레임 생성할 파티클 수
    float time; // 경과 시간
};


#endif // __PARTICLE_COMMON_HLSLI__