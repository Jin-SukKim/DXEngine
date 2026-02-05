#ifndef __PARTICLE_COMMON_HLSLI__
#define __PARTICLE_COMMON_HLSLI__

#define TOTAL_MAX_PARTICLES 10000000
#define TOTAL_MAX_EMITTERS 10000

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
    uint ownerID; // ★ 추가
};

RWStructuredBuffer<Particle> writeParticles : register(u6);
RWStructuredBuffer<uint> writeCount : register(u7);

StructuredBuffer<Particle> readParticles : register(t6);
StructuredBuffer<uint> readCount : register(t7);
Texture2DArray particleTex : register(t14);

struct EmitterID
{
    uint readParticleOffset;
    uint writeParticleOffset;
    uint emitterID;
    uint spawnPosOffset;
    uint systemSlot;
    uint padding[3];
};

struct ParticleMeshConsts
{
    matrix pWorld;
    matrix pWorldIT;
    uint vertexCount;
    uint indexCount;
    uint systemSlot;
    float padding;
};

struct ParticleFrameConsts
{
    float dt;
    float time;
    uint spawnCount;
    uint maxParticles;
};

struct SpawnConsts
{
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

struct VisualConsts
{
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

struct ForceConsts
{
    float3 velocity;
    float padding7;
    float2 speedRange;
    float2 padding8;

    float3 randomDir;
    float drag;
    float3 gravity;
    float padding9;
};

struct RenderConsts
{
    int textureIdx;
    uint frameCount;
    float2 frameTiles;
    uint indexCount;
    uint textureMode;
    int singleTextureIdx;
    uint useSorting;
};

struct VortexConsts
{
    float vortexStrength;
    float3 vortexCenter;
    float3 vortexAxis;
    float vortexFalloff;
    float2 vortexPull;
    uint active; // ★ 추가
    float padding10;
};

struct OrbitConsts
{
    float3 center;
    float rotationRate;
    float3 axis;
    float initialOffset;
    uint active; // ★ 추가
    float3 paddingOrbit;
};

struct ParticleConsts
{
    SpawnConsts spawn;
    VisualConsts visual;
    ForceConsts force;
    RenderConsts render;
    VortexConsts vortex;
    OrbitConsts orbit;
};

StructuredBuffer<ParticleFrameConsts> frameConsts : register(t8);
StructuredBuffer<ParticleConsts> consts : register(t9);
StructuredBuffer<float3> spawnPositions : register(t10);
StructuredBuffer<EmitterID> emitterIDs : register(t11);
StructuredBuffer<ParticleMeshConsts> meshConsts : register(t12);

// ★ 현재 처리 중인 Emitter ID (Spawn 시 사용)
cbuffer CurrentEmitterID : register(b5)
{
    uint currentEmitterID;
    uint3 padding_b5;
};

// ★ Helper 함수
EmitterID GetEmitterID()
{
    return emitterIDs[currentEmitterID];
}

ParticleMeshConsts GetMeshConsts()
{
    return meshConsts[emitterIDs[currentEmitterID].systemSlot];
}

struct SortElement
{
    float key;
    uint value;
};

#endif // __PARTICLE_COMMON_HLSLI__