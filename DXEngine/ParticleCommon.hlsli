#ifndef __PARTICLE_COMMON_HLSLI__
#define __PARTICLE_COMMON_HLSLI__

Texture2DArray particleTex : register(t14);

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

cbuffer ParticleConsts : register(b4) {
    float dt;
    float time;
    uint spawnCount;
    uint maxParticles;
};

cbuffer SpawnConsts : register(b5) {
    float3 localPos;
    float padding;

    float3 spawnVolume;
    float spawnInnerRatio;

    float2 lifeRange;
    int spawnShape;
    float padding1;
};

cbuffer VisualConsts : register(b6) {
    float2 sizeRange;
    float2 padding2;

    float4 startColor;
    float4 endColor;

    float3 minRotation;
    float padding3;
    float3 maxRotation;
    float padding4;
    float3 minRotSpeed;
    float padding5;
    float3 maxRotSpeed;
    float padding6;
};

cbuffer ForceConsts : register(b7) {
    float3 velocity;
    float padding7;
    float2 speedRange;
    float2 padding8;

    float3 randomDir;
    float drag;
    float3 gravity;
    float vortexStrength;
    float3 vortexCenter;
    float padding9;
    float3 vortexAxis;
    float vortexFalloff;
    float2 vortexPull;
    float2 padding10;
};

cbuffer RenderConsts : register(b8) {
    int textureIdx;
    float2 frameTiles;
    uint frameCount;
};

struct SortElement
{
    float key;
    uint value;
};

#endif // __PARTICLE_COMMON_HLSLI__