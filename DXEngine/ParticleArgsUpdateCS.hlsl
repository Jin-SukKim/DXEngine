#include "ParticleCommon.hlsli"

Buffer<uint> particleCountBuffer : register(t0);
RWBuffer<uint> dispatchArgs : register(u0);
RWBuffer<uint> drawArgs : register(u1);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint count = particleCountBuffer[0];

    dispatchArgs[0] = (count + 255) / 256;
    dispatchArgs[1] = 1;
    dispatchArgs[2] = 1;

    drawArgs[0] = count;
    drawArgs[1] = 1;
    drawArgs[2] = 0;
    drawArgs[3] = 0;
}