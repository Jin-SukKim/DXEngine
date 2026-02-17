#include "Common.hlsli"
#include "ParticleCommon.hlsli"

StructuredBuffer<uint> batchAliveIndices : register(t0);
RWStructuredBuffer<SortElement> sortBuffer : register(u0);

cbuffer SortParams : register(b5) {
    uint baseOffset;     // batchAliveIndices 내 시작 위치
    uint particleCount;  // 정렬할 파티클 수
    uint2 padding;
};

[numthreads(1024, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint id = dtID.x;

    SortElement elem;

    if (id < particleCount)
    {
        uint particleIdx = batchAliveIndices[baseOffset + id];
        Particle p = readParticles[particleIdx];

        float3 toCamera = p.position - eyeWorld;
        float distSq = dot(toCamera, toCamera);

        // BitonicSort는 내림차순 → distSq 큰 값(먼 파티클)이 앞으로 → Back-to-Front
        elem.key = distSq;
        elem.value = particleIdx;
    }
    else
    {
        // 패딩: 0은 가장 작은 값 → 내림차순에서 뒤로 밀림
        elem.key = asfloat(0);
        elem.value = 0;
    }

    sortBuffer[id] = elem;
}
