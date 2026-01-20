#include "ParticleCommon.hlsli"

Buffer<uint> particleCountBuffer : register(t0);
RWBuffer<uint> dispatchArgs : register(u0);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint count = particleCountBuffer[0];

    dispatchArgs[0] = (count + 1023) / 1024;
    dispatchArgs[1] = 1;
    dispatchArgs[2] = 1;
}