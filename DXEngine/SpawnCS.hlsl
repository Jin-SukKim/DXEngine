#include "ParticleCommon.hlsli"
#include "Common.hlsli"

struct Vertex
{
    float3 position;
    float3 normalModel;
    float2 texcoord;
    float3 tangentModel;
};

StructuredBuffer<float3> meshVertex : register(t2);
StructuredBuffer<uint> meshIndices : register(t3);
StructuredBuffer<float3> meshNormals : register(t4);
ConsumeStructuredBuffer<uint> deadIndices : register(u4);

// --- [Robust Random Functions (Wang Hash)] ---
// (���� �Լ��� �״�� ����)
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
// (���� ���� �Լ��� BoxSpawn, SphereSpawn �� �״�� ����...)
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

void VertexSpawn(inout uint rngState, uint vCount, uint vOffset, out float3 outPos)
{
    if (vCount > 0) {
        rngState = wang_hash(rngState);
        uint vIdx = rngState % vCount;
        outPos = meshVertex[vOffset + vIdx];
    }
    else {
        outPos = float3(0, 0, 0);
    }
}

void SurfaceSpawn(inout uint rngState, uint iCount, uint iOffset, uint vOffset, out float3 outPos, out float3 outNormal)
{
    if (iCount > 0) {
        uint triCount = iCount / 3;
        rngState = wang_hash(rngState);
        uint triIdx = rngState % triCount;
        uint i0 = meshIndices[iOffset + triIdx * 3 + 0];
        uint i1 = meshIndices[iOffset + triIdx * 3 + 1];
        uint i2 = meshIndices[iOffset + triIdx * 3 + 2];
        float ra = rand_float(rngState);
        float rb = rand_float(rngState);
        if (ra + rb > 1.0f) {
            ra = 1.0f - ra;
            rb = 1.0f - rb;
        }
        float3 v0 = meshVertex[vOffset + i0];
        float3 v1 = meshVertex[vOffset + i1];
        float3 v2 = meshVertex[vOffset + i2];
        outPos = v0 + ra * (v1 - v0) + rb * (v2 - v0);

        float3 n0 = meshNormals[vOffset + i0];
        float3 n1 = meshNormals[vOffset + i1];
        float3 n2 = meshNormals[vOffset + i2];
        outNormal = normalize(n0 + ra * (n1 - n0) + rb * (n2 - n0));
    }
    else {
        outPos = float3(0, 0, 0);
        outNormal = float3(0, 0, 0);
    }
}

float3 SpawnFromPositions(inout uint rngState, uint posCount, uint startIndex, uint threadIdx, bool sequential)
{
    if (posCount == 0) return float3(0, 0, 0);
    uint idx;
    if (sequential) idx = (startIndex + threadIdx) % posCount;
    else idx = rngState % posCount;
    return spawnPositions[spawnPosOffset + idx];
}

[numthreads(1024, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    if (dtID.x >= frameConsts[emitterID].spawnCount)
        return;
    
    // �õ� �ʱ�ȭ
    uint rngState = dtID.x * 1973 + uint(frameConsts[emitterID].time * 10000.0f);
    rngState = wang_hash(rngState);

    float3 spawnPos = float3(0, 0, 0);
    float3 localNormal = float3(0, 0, 0);
    rngState = wang_hash(rngState);

    SpawnConsts spawn = consts[emitterID].spawn;

    if (spawn.spawnShape == 0) spawnPos = BoxSpawn(rngState, spawn.spawnVolume, spawn.spawnInnerRatio);
    else if (spawn.spawnShape == 1) spawnPos = SphereSpawn(rngState, spawn.spawnVolume, spawn.spawnInnerRatio);
    else if (spawn.spawnShape == 2) VertexSpawn(rngState, meshConsts[systemID].vertexCount, meshConsts[systemID].vertexOffset, spawnPos);
    else if (spawn.spawnShape == 3) SurfaceSpawn(rngState, meshConsts[systemID].indexCount, meshConsts[systemID].indexOffset, meshConsts[systemID].vertexOffset, spawnPos, localNormal);
    else if (spawn.spawnShape == 4) spawnPos = SpawnFromPositions(rngState, spawn.bakedCount, 0, dtID.x, false);
    else if (spawn.spawnShape == 5) spawnPos = SpawnFromPositions(rngState, spawn.bakedCount, spawn.spawnStartIndex, dtID.x, true);

    // ��ġ �� �ӵ� ���
    Particle p;
    float3 localPos = spawnPos + spawn.localPos;
    // Surface spawn: lift particle slightly above mesh surface to prevent overlap
    if (spawn.spawnShape == 3)
        localPos += localNormal * 0.001f;

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
        matrix pWorld = meshConsts[systemID].pWorld;
        p.position = mul(float4(localPos, 1.0f), pWorld).xyz;
        p.velocity = mul(localVel, (float3x3) pWorld);
        if (length(localNormal) > 0.001f)
            p.normal = normalize(mul(localNormal, (float3x3) meshConsts[systemID].pWorldIT));
        else
            p.normal = float3(0, 0, 0);
    }
    else // Local Space
    {
        p.position = localPos;
        p.velocity = localVel;
        p.normal = localNormal;
    }

    p.life = lerp(spawn.lifeRange.x, spawn.lifeRange.y, rand_float(rngState));
    p.lifeMax = p.life;

    VisualConsts visual = consts[emitterID].visual;
    p.color = visual.startColor;
    p.size = visual.sizeRange.x;

    float toRad = 3.141592f / 180.f;
    float3 rndRot = float3(rand_float(rngState), rand_float(rngState), rand_float(rngState));
    p.rotation = lerp(visual.minRotation, visual.maxRotation, rndRot) * toRad;

    float3 rndRotSpd = float3(rand_float(rngState), rand_float(rngState), rand_float(rngState));
    p.rotSpeed = lerp(visual.minRotSpeed, visual.maxRotSpeed, rndRotSpd);

    uint particleIdx = deadIndices.Consume();

    p.ownerID = emitterID;
    p.systemID = systemID;

    particles[particleIdx] = p;

    // AliveIndices�� �߰�
    uint aliveSlot;
    InterlockedAdd(writeAliveCount[emitterID], 1, aliveSlot);
    writeAliveIndices[readParticleOffset + aliveSlot] = particleIdx;
}