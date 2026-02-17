#include "Common.hlsli"
#include "ParticleCommon.hlsli"

// eyeWorld는 Common.hlsli의 GlobalConsts (b0)에서 가져옴
// emitterID는 ParticleCommon.hlsli의 cbuffer EmitterID (b5)에서 가져옴
// readParticles는 ParticleCommon.hlsli의 t16에서 가져옴

RWStructuredBuffer<SortElement> sortedElements : register(u0);

[numthreads(1024, 1, 1)]
void main( uint3 dtID : SV_DispatchThreadID )
{
    uint id = dtID.x;
    uint count = readAliveCount[emitterID];

    SortElement elem;
    elem.value = id; // Particle index in alive list

    if (id < count)
    {
        // Get actual particle index from alive indices
        uint particleIdx = readAliveIndices[readParticleOffset + id];
        Particle p = readParticles[particleIdx];

        float3 dist = p.position - eyeWorld;
        float distSq = dot(dist, dist);
        // 거리 내림차순 정렬 (먼 것부터 → 가까운 순)
        elem.key = distSq;
    }
    else
    {
        elem.key = 3.402823466e+38F; // FLT_MAX (패딩)
    }

    sortedElements[id] = elem;
}
