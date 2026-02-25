#include "ParticleCommon.hlsli"

StructuredBuffer<SortElement> sortedElements : register(t0);
StructuredBuffer<GPUSortParams> gpuSortParams : register(t2);
RWStructuredBuffer<uint> batchAliveIndices : register(u0);

cbuffer SortGroupConsts : register(b5) {
    uint sortParamsSlot;
    float3 sgPadding;
};

[numthreads(1024, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    GPUSortParams sp = gpuSortParams[sortParamsSlot];
    uint id = dtID.x;

    uint baseOffset = sp.sortBaseOffset;
    uint particleCount = sp.sortParticleCount;

    if (id < particleCount)
    {
        // 정렬된 파티클 인덱스를 batchAliveIndices에 다시 쓰기
        uint sortedParticleIdx = sortedElements[id].value;
        batchAliveIndices[baseOffset + id] = sortedParticleIdx;
    }
}
