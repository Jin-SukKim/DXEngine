#include "ParticleCommon.hlsli"

StructuredBuffer<SortElement> sortedElements : register(t0);
RWStructuredBuffer<uint> batchAliveIndices : register(u0);

cbuffer SortParams : register(b5) {
    uint baseOffset;     // AlphaBlend 시작 offset
    uint particleCount;  // 총 AlphaBlend 파티클 수
    uint padding[2];
};

[numthreads(1024, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint id = dtID.x;

    if (id < particleCount)
    {
        // 정렬된 파티클 인덱스를 batchAliveIndices에 다시 쓰기
        uint sortedParticleIdx = sortedElements[id].value;
        batchAliveIndices[baseOffset + id] = sortedParticleIdx;
    }
}
