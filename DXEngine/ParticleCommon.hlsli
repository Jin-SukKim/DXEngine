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
    uint ownerID;
    uint systemID;
};

RWStructuredBuffer<Particle> writeParticles : register(u6);
RWStructuredBuffer<uint> writeCount : register(u7);

StructuredBuffer<Particle> readParticles : register(t6);
StructuredBuffer<uint> readCount : register(t7);
Texture2DArray particleTex : register(t14);

cbuffer EmitterID : register(b5)
{
    uint readParticleOffset;
    uint writeParticleOffset;
    uint emitterID;
    uint spawnPosOffset;  // bakedOffset + customOffset 
    uint systemID;
    float3 paddingID;
};

struct EmitterID
{
    uint readParticleOffset;
    uint writeParticleOffset;
    uint emitterID;
    uint spawnPosOffset;  // bakedOffset + customOffset 
    uint systemID;
    float3 paddingID;
};

struct ParticleFrameConsts
{
    float dt;
    float time;
    uint spawnCount;
    uint maxParticles;
    float spawnRatio;       // Current frame spawn ratio (0.0 ~ 1.0)
    float3 padding2;
};

struct SpawnConsts
{
    float3 localPos;
    float padding;

    float3 spawnVolume;
    float spawnInnerRatio;

    float2 lifeRange;
    int spawnShape;
    uint bakedCount; // Baked/Custom  
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

    // Overdraw Control - Size Scaling
    float sizeDistanceScale;      // Size multiplier at close range (e.g., 0.7 = 70%)
    float sizeDistanceMin;        // Distance where scaling starts (meters)
    float sizeDistanceMax;        // Distance where scaling ends (meters)
    uint enableSizeScaling;       // On/Off toggle (per-emitter)
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
    float padding10;
    uint active;
};

struct OrbitConsts
{
    float3 center;
    float rotationRate;
    float3 axis;
    float initialOffset;
    uint active;
    float3 orbitPadding;
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

struct ParticleMeshConsts
{
    matrix pWorld;
    matrix pWorldIT;
    uint vertexCount;
    uint indexCount;
    float2 padding;
};

StructuredBuffer<ParticleFrameConsts> frameConsts : register(t8);
StructuredBuffer<ParticleConsts> consts : register(t9);
StructuredBuffer<float3> spawnPositions : register(t10); // յ SpawnPosition 
StructuredBuffer<EmitterID> emitterIDs : register(t11); // յ SpawnPosition 
StructuredBuffer<ParticleMeshConsts> meshConsts : register(t16);

struct SortElement
{
    float key;
    uint value;
};

#endif // __PARTICLE_COMMON_HLSLI__