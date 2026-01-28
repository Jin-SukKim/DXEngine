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

cbuffer ParticleFrameConsts : register(b4) {
    float dt;
    float time;
    uint spawnCount;
    uint maxParticles;
};

struct SpawnConsts {
    float3 localPos;
    float padding;

    float3 spawnVolume;
    float spawnInnerRatio;

    float2 lifeRange;
    int spawnShape;
    uint bakedCount;
    uint simulationSpace;

    uint spawnStartIndex;
    float2 padding1;
};

struct VisualConsts {
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

struct ForceConsts {
    float3 velocity;
    float padding7;
    float2 speedRange;
    float2 padding8;

    float3 randomDir;
    float drag;
    float3 gravity;
    float padding9;
};

struct RenderConsts {
    int textureIdx;
    uint frameCount;
    float2 frameTiles;
    uint numMeshes;
    uint textureMode;
    int singleTextureIdx;
    uint useSorting;
};

struct VortexConsts {
    float vortexStrength;
    float3 vortexCenter;
    float3 vortexAxis;
    float vortexFalloff;
    float2 vortexPull;
    float2 padding10;
};

struct OrbitConsts {
    float3 center;
    float rotationRate;
    float3 axis;
    float initialOffset;
};

cbuffer ParticleConsts : register(b5) {
    SpawnConsts spawn;
    VisualConsts visual;
    ForceConsts force;
    RenderConsts render;
    VortexConsts vortex;
    OrbitConsts orbit;
};

cbuffer ParticleMeshConsts : register(b6)
{
    matrix pWorld;
    matrix pWorldIT; // World Inverse Transpose (Normal 변환에 사용)
    uint vertexCount;
    uint indexCount;
    float2 padding;
};


struct SortElement
{
    float key;
    uint value;
};

#endif // __PARTICLE_COMMON_HLSLI__