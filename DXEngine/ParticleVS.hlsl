#include "Common.hlsli"
#include "Particle.hlsli"

struct SortElement
{
    float key;
    uint value;
};

StructuredBuffer<Particle> particles : register(t0);
StructuredBuffer<SortElement> sortedElements : register(t1);

struct GSInput
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
    float life : PSIZE0;
    float size : PSIZE1;
};

GSInput main(uint vertexID : SV_VertexID)
{
    uint particleIdx = sortedElements[vertexID].value;
    Particle p = particles[particleIdx];

    GSInput output;
    output.position = float4(p.position.xyz, 1.0);
    output.color = p.color;
    output.life = p.life;
    output.size = p.size;

    return output;
}