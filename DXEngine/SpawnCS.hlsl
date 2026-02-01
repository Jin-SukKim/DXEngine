#include "ParticleCommon.hlsli"
#include "Common.hlsli"

struct Vertex {
    float3 position;
    float3 normalModel;
    float2 texcoord;
    float3 tangentModel;
};

RWStructuredBuffer<Particle> particles : register(u0);
RWStructuredBuffer<uint> activeCount : register(u1); // 카운팅용 버퍼

StructuredBuffer<float3> bakedSpawnPos : register(t0);
StructuredBuffer<float3> customSpawnPos : register(t1); 
StructuredBuffer<float3> meshVertex : register(t9);
StructuredBuffer<uint> meshIndices : register(t10);

// --- [Robust Random Functions (Wang Hash)] ---

uint wang_hash(uint seed)
{
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}

float rand_float(inout uint state)
{
    state = wang_hash(state);
    return float(state) / 4294967296.0;
}

float rand_signed(inout uint state)
{
    return rand_float(state) * 2.0f - 1.0f;
}

// --- [Spawn Functions] ---

float3 BoxSpawn(inout uint rngState, float3 volume, float innerRatio)
{
    float3 pos;
    pos.x = rand_signed(rngState);
    pos.y = rand_signed(rngState);
    pos.z = rand_signed(rngState);

    float3 posAbs = abs(pos);
    float3 hollowScale = lerp(innerRatio, 1.0f, posAbs);

    return sign(pos) * hollowScale * volume;
}

float3 SphereSpawn(inout uint rngState, float3 volume, float innerRatio)
{
    float theta = rand_float(rngState) * 6.28318530718f;
    float z = rand_float(rngState) * 2.0f - 1.0f;
    float r = sqrt(max(0.0f, 1.0f - z * z));

    float3 dir = float3(r * cos(theta), r * sin(theta), z);
    float dist = lerp(innerRatio, 1.0f, rand_float(rngState));

    return dir * dist * volume;
}

// Vertex 스폰
void VertexSpawn(inout uint rngState, uint vCount, out float3 outPos)
{
    if (vCount > 0)
    {
        rngState = wang_hash(rngState);
        uint vIdx = rngState % vCount;
        outPos = meshVertex[vIdx];
    }
    else
    {
        outPos = float3(0, 0, 0);
    }
}

// Surface 스폰
void SurfaceSpawn(inout uint rngState, uint iCount, out float3 outPos)
{
    if (iCount > 0)
    {
        uint triCount = iCount / 3;
        rngState = wang_hash(rngState);
        uint triIdx = rngState % triCount;

        uint i0 = meshIndices[triIdx * 3 + 0];
        uint i1 = meshIndices[triIdx * 3 + 1];
        uint i2 = meshIndices[triIdx * 3 + 2];

        float ra = rand_float(rngState);
        float rb = rand_float(rngState);

        if (ra + rb > 1.0f)
        {
            ra = 1.0f - ra;
            rb = 1.0f - rb;
        }

        float3 v0 = meshVertex[i0];
        float3 v1 = meshVertex[i1];
        float3 v2 = meshVertex[i2];

        outPos = v0 + ra * (v1 - v0) + rb * (v2 - v0);
    }
    else
    {
        outPos = float3(0, 0, 0);
    }
}

[numthreads(1024, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    if (dtID.x >= frameConsts[emitterID].spawnCount)
        return;

    if (activeCount[emitterID] >= frameConsts[emitterID].maxParticles)
        return;

    // 시드 초기화
    uint rngState = dtID.x * 1973 + uint(frameConsts[emitterID].time * 10000.0f);
    rngState = wang_hash(rngState);

    Particle p;
    float3 spawnPos = float3(0, 0, 0);

    // 매 시도마다 시드 갱신
    rngState = wang_hash(rngState);

    SpawnConsts spawn = consts[emitterID].spawn;
    if (spawn.spawnShape == 0) // BOX
        spawnPos = BoxSpawn(rngState, spawn.spawnVolume, spawn.spawnInnerRatio);
    else if (spawn.spawnShape == 1) // SPHERE
        spawnPos = SphereSpawn(rngState, spawn.spawnVolume, spawn.spawnInnerRatio);
    else if (spawn.spawnShape == 2) // VERTEX
        VertexSpawn(rngState, vertexCount, spawnPos);
    else if (spawn.spawnShape == 3) // SURFACE
        SurfaceSpawn(rngState, indexCount, spawnPos);
    else if (spawn.spawnShape == 4) // BAKED POS
    {
        uint idx = rngState % spawn.bakedCount;
        spawnPos = bakedSpawnPos[bakedOffset + idx];
    }
    else if (spawn.spawnShape == 5)
    {
        uint idx = (spawn.spawnStartIndex + dtID.x) % spawn.bakedCount;
        spawnPos = bakedSpawnPos[idx]; // customOffset 제거 (CPU에서 이미 올바른 버퍼 바인딩)
    }

    // 위치 및 속도 설정
    float3 localPos = spawnPos + spawn.localPos;

    float3 noiseDir;
    noiseDir.x = rand_signed(rngState);
    noiseDir.y = rand_signed(rngState);
    noiseDir.z = rand_signed(rngState);

    ForceConsts force = consts[emitterID].force;
    float3 finalDir = normalize(force.velocity + noiseDir * force.randomDir + 1e-5f);
    float speed = lerp(force.speedRange.x, force.speedRange.y, rand_float(rngState));
    float3 localVel = finalDir * speed;

    if (spawn.simulationSpace == 1) // World Space
    {
        p.position = mul(float4(localPos, 1.0f), pWorld).xyz;
        p.velocity = mul(localVel, (float3x3)pWorld);
    }
    else // Local Space
    {
        p.position = localPos;
        p.velocity = localVel;
    }

    // Life
    p.life = lerp(spawn.lifeRange.x, spawn.lifeRange.y, rand_float(rngState));
    p.lifeMax = p.life;

    VisualConsts visual = consts[emitterID].visual;
    // Color & Size
    p.color = visual.startColor;
    p.size = visual.sizeRange.x;

    float toRad = 3.141592f / 180.f;

    // Rotation
    float3 rndRot = float3(rand_float(rngState), rand_float(rngState), rand_float(rngState));
    p.rotation = lerp(visual.minRotation, visual.maxRotation, rndRot) * toRad;

    // RotSpeed
    float3 rndRotSpd = float3(rand_float(rngState), rand_float(rngState), rand_float(rngState));
    p.rotSpeed = lerp(visual.minRotSpeed, visual.maxRotSpeed, rndRotSpd);

    // counts[0]을 1 증가시키고, '증가되기 전의 값'을 index에 받아옵니다.
    uint index;
    InterlockedAdd(activeCount[emitterID], 1, index);
    particles[particleOffset + index] = p;
}