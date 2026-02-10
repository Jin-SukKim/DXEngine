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

// --- [Robust Random Functions (Wang Hash)] ---
// (기존 함수들 그대로 유지)
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
// (기존 스폰 함수들 BoxSpawn, SphereSpawn 등 그대로 유지...)
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

void VertexSpawn(inout uint rngState, uint vCount, out float3 outPos)
{
    if (vCount > 0) {
        rngState = wang_hash(rngState);
        uint vIdx = rngState % vCount;
        outPos = meshVertex[vIdx];
    }
    else {
        outPos = float3(0, 0, 0);
    }
}

void SurfaceSpawn(inout uint rngState, uint iCount, out float3 outPos)
{
    if (iCount > 0) {
        uint triCount = iCount / 3;
        rngState = wang_hash(rngState);
        uint triIdx = rngState % triCount;
        uint i0 = meshIndices[triIdx * 3 + 0];
        uint i1 = meshIndices[triIdx * 3 + 1];
        uint i2 = meshIndices[triIdx * 3 + 2];
        float ra = rand_float(rngState);
        float rb = rand_float(rngState);
        if (ra + rb > 1.0f) {
            ra = 1.0f - ra;
            rb = 1.0f - rb;
        }
        float3 v0 = meshVertex[i0];
        float3 v1 = meshVertex[i1];
        float3 v2 = meshVertex[i2];
        outPos = v0 + ra * (v1 - v0) + rb * (v2 - v0);
    }
    else {
        outPos = float3(0, 0, 0);
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

// ==========================================================
// [LDS 최적화 적용]
// ==========================================================
groupshared uint g_GroupSpawnCount; // 그룹 내에서 생성될 파티클 수
groupshared uint g_GlobalBaseIndex; // 전역 버퍼 할당 시작 인덱스

[numthreads(1024, 1, 1)]
void main(uint3 gID : SV_GroupID, uint3 gtID : SV_GroupThreadID, uint3 dtID : SV_DispatchThreadID)
{
    // 1. 그룹 공유 메모리 초기화 (대표 스레드 1명이 수행)
    if (gtID.x == 0)
    {
        g_GroupSpawnCount = 0;
        g_GlobalBaseIndex = 0;
    }
    GroupMemoryBarrierWithGroupSync(); // 모든 스레드 대기

    // ------------------------------------------------------
    // 2. 스폰 로직 계산 (로컬 변수에 저장)
    // ------------------------------------------------------
    bool shouldSpawn = false;
    Particle p = (Particle)0;

    // 유효성 체크: 기존의 return 대신 플래그(shouldSpawn)를 사용
    // (LDS 동기화를 위해 모든 스레드가 끝까지 실행되어야 함)
    if (dtID.x < frameConsts[emitterID].spawnCount)
    {
        shouldSpawn = true;

        // 시드 초기화
        uint rngState = dtID.x * 1973 + uint(frameConsts[emitterID].time * 10000.0f);
        rngState = wang_hash(rngState);

        float3 spawnPos = float3(0, 0, 0);
        rngState = wang_hash(rngState);

        SpawnConsts spawn = consts[emitterID].spawn;

        if (spawn.spawnShape == 0) spawnPos = BoxSpawn(rngState, spawn.spawnVolume, spawn.spawnInnerRatio);
        else if (spawn.spawnShape == 1) spawnPos = SphereSpawn(rngState, spawn.spawnVolume, spawn.spawnInnerRatio);
        else if (spawn.spawnShape == 2) VertexSpawn(rngState, meshConsts[p.systemID].vertexCount, spawnPos);
        else if (spawn.spawnShape == 3) SurfaceSpawn(rngState, meshConsts[p.systemID].indexCount, spawnPos);
        else if (spawn.spawnShape == 4) spawnPos = SpawnFromPositions(rngState, spawn.bakedCount, 0, dtID.x, false);
        else if (spawn.spawnShape == 5) spawnPos = SpawnFromPositions(rngState, spawn.bakedCount, spawn.spawnStartIndex, dtID.x, true);

        // 위치 및 속도 계산
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
            matrix pWorld = meshConsts[systemID].pWorld;
            p.position = mul(float4(localPos, 1.0f), pWorld).xyz;
            p.velocity = mul(localVel, (float3x3) pWorld);
        }
        else // Local Space
        {
            p.position = localPos;
            p.velocity = localVel;
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
    }

    // ------------------------------------------------------
    // 3. LDS 최적화 (그룹 카운팅 & 전역 예약)
    // ------------------------------------------------------
    uint localIndex = 0;

    // A. 그룹 내 카운팅 (매우 빠름)
    if (shouldSpawn)
    {
        InterlockedAdd(g_GroupSpawnCount, 1, localIndex);
    }
    GroupMemoryBarrierWithGroupSync(); // 그룹 집계 완료 대기

    // B. 전역 공간 예약 (그룹 대표 1명만 수행)
    if (gtID.x == 0 && g_GroupSpawnCount > 0)
    {
        // "우리 그룹 N명 들어갑니다. 자리 주세요."
        InterlockedAdd(writeCount[emitterID], g_GroupSpawnCount, g_GlobalBaseIndex);
    }
    GroupMemoryBarrierWithGroupSync(); // 전역 오프셋 받아올 때까지 대기

    // ------------------------------------------------------
    // 4. 최종 쓰기
    // ------------------------------------------------------
    if (shouldSpawn)
    {
        // 최종 전역 인덱스 계산
        uint finalIndex = g_GlobalBaseIndex + localIndex;

        // [중요] 최대 파티클 개수 초과 방지 (여기서 체크)
        // 예약은 했지만(Atomic은 되돌릴 수 없으므로), 범위를 넘어가면 쓰지 않고 버림
        if (finalIndex < frameConsts[emitterID].maxParticles)
        {
            p.ownerID = emitterID;
            p.systemID = systemID;
            writeParticles[writeParticleOffset + finalIndex] = p;
        }
    }
}