#include "ParticleCommon.hlsli"

AppendStructuredBuffer<Particle> outputParticles : register(u0);
Buffer<uint> activeCount : register(t0);

[numthreads(10, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    // CPU가 계산한 정확한 개수만큼만 생성
    if (dtID.x >= 10)
        return;
    
    // 버퍼 오버플로우 방지 (GPU가 체크)
    uint currentCount = activeCount[0];
    if (currentCount >= maxParticles) // 최대 파티클 수
        return;
    
    Particle p;
    
    p.position = float3(0.0f, 0.0f, 0.0f);
    p.velocity = float3(0.0f, 5.0f, 0.0f);
    p.color = float3(1.0f, 0.0f, 0.0f);
    p.life = 2.0f;
    p.size = 0.05f;
    
    outputParticles.Append(p);
}