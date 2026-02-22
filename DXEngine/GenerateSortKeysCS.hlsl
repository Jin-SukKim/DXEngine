#include "Common.hlsli"
#include "ParticleCommon.hlsli"

StructuredBuffer<uint> batchAliveIndices : register(t0);
RWStructuredBuffer<SortElement> sortBuffer : register(u0);

cbuffer SortParams : register(b5) {
    uint sortBaseOffset;
    uint sortParticleCount;
    uint2 padding;
    float3 cameraForward;
    float pad1;
};

[numthreads(1024, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint id = dtID.x;

    uint baseOffset = sortBaseOffset;
    uint particleCount = sortParticleCount;

    SortElement elem;

    if (id < particleCount)
    {
        uint particleIdx = batchAliveIndices[baseOffset + id];
        Particle p = readParticles[particleIdx];

        // Planar depth: dot product with camera forward
        float3 toParticle = p.position - eyeWorld;
        float viewZ = dot(toParticle, cameraForward);

        elem.key = viewZ;
        elem.value = particleIdx;
    }
    else
    {
        // 내림차순 정렬에서 확실히 뒤로 밀리도록 -FLT_MAX
        elem.key = -3.402823466e+38f;
        elem.value = 0xFFFFFFFF;
    }

    sortBuffer[id] = elem;
}
