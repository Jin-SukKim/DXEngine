#include "ParticleCommon.hlsli"

Buffer<uint> activeCount : register(t0);
ConsumeStructuredBuffer<Particle> inputParticles : register(u0);
AppendStructuredBuffer<Particle> outputParticles : register(u1);

[numthreads(256, 1, 1)]
void main(uint3 gID : SV_GroupID, int3 gtID : SV_GroupThreadID, uint3 dtID : SV_DispatchThreadID)
{
    if (dtID.x >= activeCount[0])
        return;

    Particle p = inputParticles.Consume();
    
    p.position += p.velocity * dt;
    p.velocity.y -= 9.8f * dt;
    p.life -= dt * 0.3f;
    
    if (p.position.y < -20.0f)
    {
        p.life = 0.0f;
    }
    
    // 살아있는 파티클만 출력
    if (p.life > 0.0f)
    {
        outputParticles.Append(p);
    }
}