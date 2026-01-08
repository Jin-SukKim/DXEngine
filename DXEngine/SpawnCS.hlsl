#include "ParticleCommon.hlsli"

AppendStructuredBuffer<Particle> outputParticles : register(u0);

[numthreads(10, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    Particle p;
    
    p.position = float3(0.0f, 0.0f, 0.0f);
    p.velocity = float3(0.0f, 5.0f, 0.0f);
    p.color = float3(1.0f, 0.0f, 0.0f);
    p.life = 2.0f;
    p.size = 0.05f;
    
    outputParticles.Append(p);
}