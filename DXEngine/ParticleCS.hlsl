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

    // 위치 업데이트
    p.position += p.velocity * dt;
    
    // 중력 적용
    p.velocity.y -= 9.8f * dt;
    
    // 수명 감소
    p.life -= dt * 0.5f; // ?? 천천히 감소
    
    // 화면 밖으로 나가면 제거
    if (p.position.y < -10.0f)
    {
        p.life = 0.0f;
    }
    
    // 살아있는 파티클만 출력
    if (p.life > 0.0f)
    {
        outputParticles.Append(p);
    }
}