#include "ParticleCommon.hlsli"

RWBuffer<uint> dispatchArgs : register(u0);
RWBuffer<uint> billboardArgs : register(u1);
RWBuffer<uint> meshArgs : register(u2);

[numthreads(256, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint myEmitterID = DTid.x;

    if (myEmitterID >= TOTAL_MAX_EMITTERS)
        return;

    uint count = readCount[myEmitterID];
    
    // ★ readParticleOffset 사용 (렌더링 시 읽기 위치)
    uint readOffset = emitterIDs[myEmitterID].readParticleOffset;

    // 1. Dispatch Args
    uint dispatchIdx = myEmitterID * 3;
    dispatchArgs[dispatchIdx + 0] = (count + 255) / 256;
    dispatchArgs[dispatchIdx + 1] = 1;
    dispatchArgs[dispatchIdx + 2] = 1;

    // 2. Billboard Args
    uint drawIdx = myEmitterID * 4;
    billboardArgs[drawIdx + 0] = count;
    billboardArgs[drawIdx + 1] = 1;
    billboardArgs[drawIdx + 2] = readOffset;
    billboardArgs[drawIdx + 3] = 0;

    // 3. Mesh Args
    uint meshIdx = myEmitterID * 5;
    meshArgs[meshIdx + 0] = consts[myEmitterID].render.indexCount;
    meshArgs[meshIdx + 1] = count;
    meshArgs[meshIdx + 2] = 0;
    meshArgs[meshIdx + 3] = 0;
    meshArgs[meshIdx + 4] = readOffset;
}