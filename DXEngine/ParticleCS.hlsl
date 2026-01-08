#include "ParticleCommon.hlsli"

Buffer<uint> activeCount : register(t0);
ConsumeStructuredBuffer<Particle> inputParticles : register(u0);
AppendStructuredBuffer<Particle> outputParticles : register(u1);

[numthreads(256, 1, 1)]
void main(uint3 gID : SV_GroupID, int3 gtID : SV_GroupThreadID, uint3 dtID : SV_DispatchThreadID)
{
    // 유효 범위를 벗어나면 리턴
    if (dtID.x >= activeCount[0])
        return;
    
    Particle p = inputParticles.Consume();

    // 업데이트 (주석 해제!)
    p.life -= dt; // 수명 감소
    p.position += p.velocity * dt; // 이동 logic
    
    // 수명이 0보다 클 때만 Append (죽은 파티클은 Append 안 함 -> 자연 소멸)
    if (p.life > 0.0f)
    {
        outputParticles.Append(p);
    }
}