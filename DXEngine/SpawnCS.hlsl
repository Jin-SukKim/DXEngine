#include "ParticleCommon.hlsli"

AppendStructuredBuffer<Particle> outputParticles : register(u0);
Buffer<uint> activeCount : register(t0);

[numthreads(256, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    // 이번 프레임에 생성해야 할 개수(spawnCount)를 넘으면 생성 중단
    if (dtID.x >= (uint) spawnCount)
        return;
    
    Particle p;
    
    p.position = float3(0.0f, 0.0f, 0.0f);
    p.velocity = float3(0.0f, 5.0f, 0.0f);
    p.color = float3(1.0f, 0.0f, 0.0f);
    p.life = 1.0f;
    p.size = 0.02f;
    
    outputParticles.Append(p);
}