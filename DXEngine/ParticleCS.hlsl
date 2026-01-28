#include "ParticleCommon.hlsli"

RWStructuredBuffer<Particle> particles : register(u0);
RWStructuredBuffer<uint> activeCount : register(u1);

[numthreads(1024, 1, 1)]
void main(uint3 gID : SV_GroupID, int3 gtID : SV_GroupThreadID, uint3 dtID : SV_DispatchThreadID)
{
    // 유효 범위를 벗어나면 리턴
    if (dtID.x >= activeCount[0])
        return;
    
    Particle p = particles[dtID.x];
    
    if (p.life - dt > 0.f) {
        p.life -= dt;
        
        // 1. 물리 연산 (Physics)

        // 중력 적용
        p.velocity += force.gravity * dt;

        // 공기 저항 (Drag) 적용
        // drag 값이 클수록 속도가 0에 빠르게 수렴
        p.velocity *= 1.0f / (1.0f + force.drag * dt);

        // 위치 갱신
        p.position += p.velocity * dt;

        // 2. 시각 효과 (Visuals)

        float ageRatio = 1.0f - (p.life / max(p.lifeMax, 0.0001f));
        // 크기 보간 (Start -> End)
        // sizeRange.x = Start Size, sizeRange.y = End Size
        p.size = lerp(visual.sizeRange.x, visual.sizeRange.y, ageRatio);

        // 색상 보간 (Start -> End)
        p.color = lerp(visual.startColor, visual.endColor, ageRatio);

        //p.rotation += p.rotSpeed * dt;
        p.rotation = fmod(p.rotation + p.rotSpeed * dt, 6.28318530718f);

        // 결과 저장
        particles[dtID.x] = p;
    }
    else {
        // --- [사망 처리: Swap & Pop] ---
        // 카운터를 1 감소시키고, "줄어들기 전의 값"을 가져옴
        uint originalCount;
        InterlockedAdd(activeCount[0], -1, originalCount);

        // 마지막 파티클의 인덱스 계산 (개수가 줄었으므로 -1)
        uint lastIndex = originalCount - 1;

        // 내가 마지막 파티클이 아니라면, 마지막 파티클을 내 자리로 복사
        if (dtID.x != lastIndex)
        {
            // 주의: 멀티스레드 환경에서 lastIndex의 파티클도 동시에 업데이트 중일 수 있음.
            // 완벽한 동기화를 위해서는 복잡해지지만, 
            // 시각적 효과용 파티클에서는 보통 마지막 파티클의 이전 프레임 데이터를 복사해도 무방함.
            Particle lastP = particles[lastIndex];
            particles[dtID.x] = lastP;
        }
    }
}