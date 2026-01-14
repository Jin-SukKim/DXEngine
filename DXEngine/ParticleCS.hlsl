#include "ParticleCommon.hlsli"

Buffer<uint> activeCount : register(t0);
ConsumeStructuredBuffer<Particle> inputParticles : register(u0);
AppendStructuredBuffer<Particle> outputParticles : register(u1);

float3 CalculateVortexForce(float3 pos, float3 axis, float pull) {
    float3 fromCenter = pos - vortexCenter;

    // 회전축에 투영된 벡터를 제거 -> 회전 평면 벡터 (회전축에 수직인 벡터)
    float3 projected = fromCenter - dot(fromCenter, axis) * axis;
    float dist = length(projected); // 중심과의 거리

    if (dist < 0.0001) return float3(0, 0, 0);

    float3 dir = normalize(projected);
    float3 tangent = cross(axis, dir); // 회전 방향
    
    // Tangent Force(회전) + Radial Force(구심력/원심력)
    // Strength: 회전 속도 및 방향 (+: 시계, -: 반시계)
    // Pull: 중심으로 당기는 힘 (+: 당김, -: 확산)
    return (tangent * vortexStrength) - (dir * pull);
}

[numthreads(256, 1, 1)]
void main(uint3 gID : SV_GroupID, int3 gtID : SV_GroupThreadID, uint3 dtID : SV_DispatchThreadID)
{
    // 유효 범위를 벗어나면 리턴
    if (dtID.x >= activeCount[0])
        return;
    
    Particle p = inputParticles.Consume();
    
    if (p.life - dt > 0.f) {
        p.life -= dt;
        
        // 1. 물리 연산 (Physics)

        // Vortex(소용돌이)        
        // 생존 비율 (0.0: 탄생 직후 ~ 1.0: 사망 직전)
        // 주의: p.life는 줄어드므로 (Max -> 0), 1 - (life/lifeMax) 해야 0 -> 1 로 흐름
        float ageRatio = 1.0f - (p.life / p.lifeMax);

        // 시간 흐름에 따라 Pull 힘을 보간 (Start -> End)
        // 예: -20 (퍼짐) -> +50 (모임)
        float currentPull = lerp(vortexPull[0], vortexPull[1], ageRatio);

        if (abs(vortexStrength) > 0.001 || abs(currentPull) > 0.001) {
            float3 vForce = CalculateVortexForce(p.position, normalize(vortexAxis), currentPull);
            p.velocity += vForce * dt;
        }

        // 중력 적용
        p.velocity += gravity * dt;

        // 공기 저항 (Drag) 적용
        // drag 값이 클수록 속도가 0에 빠르게 수렴
        p.velocity *= max(0.0f, 1.0f - drag * dt);

        // 위치 갱신
        p.position += p.velocity * dt;

        // 2. 시각 효과 (Visuals)


        // 크기 보간 (Start -> End)
        // sizeRange.x = Start Size, sizeRange.y = End Size
        p.size = lerp(sizeRange.x, sizeRange.y, ageRatio);

        // 색상 보간 (Start -> End)
        p.color = lerp(startColor, endColor, ageRatio);

        p.rotation += p.rotSpeed * dt;

        // 결과 저장
        outputParticles.Append(p);
    }
}