#ifndef __PARTICLE_COMMON_HLSLI__
#define __PARTICLE_COMMON_HLSLI__

static const uint BLOCK_SIZE = 1024;

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

RWStructuredBuffer<Particle> writeParticles : register(u6);
RWStructuredBuffer<uint> writeCount : register(u7);

StructuredBuffer<Particle> readParticles : register(t6);
StructuredBuffer<uint> readCount : register(t7);
Texture2DArray particleTex : register(t14);
StructuredBuffer<uint> pageTable : register(t16);

cbuffer EmitterID : register(b5)
{
    uint pageTableOffset;
    uint blockCount;
    uint emitterID;
    uint spawnPosOffset;  // bakedOffset + customOffset 통합
};

uint GetPageTableIndex(uint local) {
    // ====================================================
    // [페이징 주소 변환]
    // ====================================================

    // 나는 몇 번째 블록에 속해 있는가?
    // 예: ID 1500, BlockSize 1024 -> 1번째 블록 (인덱스 1)
    uint logicalBlockIdx = local / BLOCK_SIZE;

    // 블록 내에서 나는 몇 번째 칸인가?
    // 예: ID 1500 -> 1024개 빼고 476번째 칸
    uint offsetInBlock = local % BLOCK_SIZE;

    // [지도 조회] 전역 테이블에서 '진짜 물리 블록 번호'를 찾아온다.
    // 위치 = 내 시스템의 시작점(PageTableOffset) + 나의 블록 순서(logicalBlockIdx)
    uint physicalBlockID = pageTable[pageTableOffset + logicalBlockIdx];

    // 최종 물리 메모리 주소 계산
    // 진짜 블록 위치로 가서(physicalBlockID * BlockSize) + 칸만큼 이동(offsetInBlock)
    uint realAddress = (physicalBlockID * BLOCK_SIZE) + offsetInBlock;
    return realAddress;
}
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
    uint bakedCount; // Baked/Custom 공용 개수
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
    float2 padding10;
};

struct OrbitConsts
{
    float3 center;
    float rotationRate;
    float3 axis;
    float initialOffset;
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
StructuredBuffer<float3> spawnPositions : register(t10); // 통합된 SpawnPosition 버퍼

cbuffer ParticleMeshConsts : register(b6)
{
    matrix pWorld;
    matrix pWorldIT;
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