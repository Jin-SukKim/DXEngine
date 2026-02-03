#include "Common.hlsli"
#include "ParticleCommon.hlsli"

// TODO: particle pos에 world matrix 곱해주기

StructuredBuffer<Particle> particles : register(t0);
Buffer<uint> activeCount : register(t1);

RWStructuredBuffer<SortElement> sortedElements : register(u0);

[numthreads(1024, 1, 1)]
void main( uint3 dtID : SV_DispatchThreadID )
{
    uint id = dtID.x;
    uint count = activeCount[readEmitterID];
    
    SortElement elem;
    elem.value = id; // Particle index
    
    if (id < count)
    {
        float3 dist = particles[id].position - eyeWorld;
        float distSq = dot(dist, dist);
        // 거리값이 같으면 정렬 순서가 불안정해 깜빡거리는 것처럼 보이는걸
        // ID로 작은 가중치를 더해 Stable Sort를 유도
        elem.key = distSq + ((float)id + 0.01f); 
    } else
    {
        elem.key = -1.f;
    }

    sortedElements[id] = elem;
}