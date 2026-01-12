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
    
    p.life -= dt;

    if (p.life > 0.f) {
        float3 acceleration = gravity;

        p.velocity += acceleration * dt; // 지소적인 가속
        p.velocity *= max(0.f, 1.f - drag * dt); // 저항값으로 속도를 줄이는 역할

        p.position += p.velocity * dt;

        float ratio = p.life / p.lifeMax; // [0.0, 1.0] -> life는 1에서 0으로 감소
        p.size = lerp(minMaxSize[1], minMaxSize[0], ratio);
        p.color = lerp(endColor, startColor, ratio);

        outputParticles.Append(p);
    }
}