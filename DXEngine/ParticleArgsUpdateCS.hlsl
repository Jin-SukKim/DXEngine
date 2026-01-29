#include "ParticleCommon.hlsli"

StructuredBuffer<uint> activeCount : register(t0);
RWBuffer<uint> dispatchArgs : register(u0);  // uint 배열로 접근

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint count = activeCount[emitterID];

    // 각 Emitter의 DispatchArgs 시작 위치 (3개의 uint per emitter)
    uint baseIdx = emitterID * 3;

    dispatchArgs[baseIdx + 0] = (count + 1023) / 1024;  // threadGroupCountX
    dispatchArgs[baseIdx + 1] = 1;                       // threadGroupCountY
    dispatchArgs[baseIdx + 2] = 1;                       // threadGroupCountZ
}