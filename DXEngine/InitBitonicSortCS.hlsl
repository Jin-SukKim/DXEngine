#include "Common.hlsli"

struct Particle
{
    float3 position;
    float3 velocity;
    float3 color;
    float life;
    float lifeMax;
    float size;
    float rotation;
    float rotSpeed;
};

struct SortElement
{
    float key;
    uint value;
};

// TODO: particle posø° world matrix ∞ˆ«ÿ¡÷±‚

StructuredBuffer<Particle> particles : register(t0);
Buffer<uint> activeCount : register(t1);

RWStructuredBuffer<SortElement> sortedElements : register(u0);

[numthreads(1024, 1, 1)]
void main( uint3 dtID : SV_DispatchThreadID )
{
    uint id = dtID.x;
    uint count = activeCount[0];
    
    SortElement elem;
    elem.value = id; // Particle index
    
    if (id < count)
    {
        float3 dist = particles[id].position - eyeWorld;
        elem.key = dot(dist, dist);
    } else
    {
        elem.key = -1.f;
    }

    sortedElements[id] = elem;
}