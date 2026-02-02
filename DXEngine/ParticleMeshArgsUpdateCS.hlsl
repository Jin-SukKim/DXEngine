// [ParticleMeshArgsUpdateCS.hlsl]
#include "ParticleCommon.hlsli"

RWBuffer<uint> drawArgs : register(u0);

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
}