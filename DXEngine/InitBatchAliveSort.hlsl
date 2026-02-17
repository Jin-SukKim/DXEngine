#include "Common.hlsli"
#include "ParticleCommon.hlsli"

StructuredBuffer<uint> batchAliveIndices : register(t0);
RWStructuredBuffer<SortElement> sortElements : register(u0);

cbuffer SortParams : register(b5) {
    uint baseOffset;     // AlphaBlend 시작 offset
    uint particleCount;  // 총 AlphaBlend 파티클 수
    uint padding[2];
};

[numthreads(1024, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint id = dtID.x;

    SortElement elem;
    elem.value = id;

    if (id < particleCount)
    {
        // batchAliveIndices[baseOffset + id]에서 실제 파티클 인덱스 가져오기
        uint globalParticleIdx = batchAliveIndices[baseOffset + id];
        Particle p = readParticles[globalParticleIdx];

        float3 dist = p.position - eyeWorld;
        float distSq = dot(dist, dist);

        // 거리 내림차순 정렬 (먼 것부터 → 가까운 순)
        elem.key = distSq;
        elem.value = globalParticleIdx; // 원본 파티클 인덱스 저장
    }
    else
    {
        elem.key = 3.402823466e+38F; // FLT_MAX (패딩)
        elem.value = 0;
    }

    sortElements[id] = elem;
}
