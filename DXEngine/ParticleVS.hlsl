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

StructuredBuffer<Particle> particles : register(t0);

struct GSInput
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
    float life : PSIZE0;
    float size : PSIZE1;
};

GSInput main(uint vertexID : SV_VertexID)
{
    Particle p = particles[vertexID];

    GSInput output;
    output.position = float4(p.position.xyz, 1.0);
    output.color = p.color;
    output.life = p.life;
    output.size = p.size;

    return output;
}