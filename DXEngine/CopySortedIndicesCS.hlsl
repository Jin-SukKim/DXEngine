#include "ParticleCommon.hlsli"

StructuredBuffer<SortElement> sortedElements : register(t0);
RWStructuredBuffer<uint> batchAliveIndices : register(u0);

cbuffer SortParams : register(b5) {
    uint sortBaseOffset;
    uint sortParticleCount;
    uint2 padding;
};

[numthreads(1024, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint id = dtID.x;

    uint baseOffset = sortBaseOffset;
    uint particleCount = sortParticleCount;

    if (id < particleCount)
    {
        // 정렬된 파티클 인덱스를 batchAliveIndices에 다시 쓰기
        uint sortedParticleIdx = sortedElements[id].value;
        batchAliveIndices[baseOffset + id] = sortedParticleIdx;
    }
}
