#ifndef __PARTICLE_COMMON_HLSLI__
#define __PARTICLE_COMMON_HLSLI__

struct Particle
{
    float3 position;
    float3 velocity;
    float4 color;
    float life;
    float lifeMax;
    float size;
    float3 rotation;
    float3 rotSpeed;
};

cbuffer ParticleConsts : register(b4)
{
    // Block 1: 기본 정보 (16 bytes)
    float dt;
    float time;
    uint spawnCount;
    uint maxParticles;

    float3 localPos;
    float padding0;
    float3 spawnVolume;
    float spawnInnerRatio;
    float2 lifeRange;
    int spawnShape; // 0: Box, 1: Sphere
    float padding1;

    float3 velocity;
    float padding2;
    float2 speedRange;
    float2 padding3;

    float3 randomDir;
    float drag;

    float3 gravity;
    float vortexStrength;

    float3 vortexCenter;
    float padding4;
    float3 vortexAxis;
    float vortexFalloff;
    float2 vortexPull;
    float2 sizeRange;

    float4 startColor;
    float4 endColor;

    float3 minRotation;
    float padding6;
    float3 maxRotation;
    float padding7;
    float3 minRotSpeed;
    float padding8;
    float3 maxRotSpeed;
    float padding9;
};

struct SortElement
{
    float key;
    uint value;
};

#endif // __PARTICLE_COMMON_HLSLI__