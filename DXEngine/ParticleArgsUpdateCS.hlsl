#include "ParticleCommon.hlsli"

RWBuffer<uint> dispatchArgs : register(u0); // per-emitter (DispatchIndirect)
RWBuffer<uint> batchDispatchArgs : register(u1); // 배치 dispatch용

[numthreads(256, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint myEmitterID = DTid.x;
    uint count = readAliveCount[myEmitterID];
    uint spawnCount = frameConsts[myEmitterID].spawnCount;

    // Per-emitter dispatch args (SpawnCS 등에서 사용)
    uint dispatchIdx = myEmitterID * 3;
    dispatchArgs[dispatchIdx + 0] = (spawnCount + 1023) / 1024;
    dispatchArgs[dispatchIdx + 1] = 1;
    dispatchArgs[dispatchIdx + 2] = 1;

    // 배치 dispatch args: 전체 중 최대 endIndex 기반으로 그룹 수 계산
    uint totalExpected = count + spawnCount;
    if (totalExpected > 0)
    {
        uint endIndex = emitterIDs[myEmitterID].readParticleOffset + totalExpected;
        uint batchGroups = (endIndex + 1023) / 1024;
        InterlockedMax(batchDispatchArgs[0], batchGroups);
    }

    // Y, Z는 1로 설정
    if (DTid.x == 0)
    {
        batchDispatchArgs[1] = 1;
        batchDispatchArgs[2] = 1;
    }
}