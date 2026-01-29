#include "ParticleCommon.hlsli"

StructuredBuffer<uint> activeCount : register(t0);
RWBuffer<uint> dispatchArgs : register(u0);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint count = activeCount[emitterID];

    dispatchArgs[0] = (count + 1023) / 1024;
    dispatchArgs[1] = 1;
    dispatchArgs[2] = 1;
}