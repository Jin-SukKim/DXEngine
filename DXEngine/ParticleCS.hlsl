#include "ParticleCommon.hlsli"
#include "OrbitCS.hlsli"
#include "VortexCS.hlsli"

// [1] 그룹 공유 메모리 선언
groupshared uint g_GroupAliveCount; // 그룹 내 생존자 수
groupshared uint g_GlobalBaseIndex; // 전역 버퍼에서 할당받은 시작 인덱스

[numthreads(1024, 1, 1)]
void main(uint3 gID : SV_GroupID, int3 gtID : SV_GroupThreadID, uint3 dtID : SV_DispatchThreadID)
{
    // [2] 공유 메모리 초기화 (그룹 대표 1명만 수행)
    if (gtID.x == 0)
    {
        g_GroupAliveCount = 0;
        g_GlobalBaseIndex = 0;
    }
    GroupMemoryBarrierWithGroupSync(); // 모든 스레드가 여기서 대기

    // ----------------------------------------------------
    // 로직 수행 (기존 코드)
    // 주의: Barrier 동기화를 위해 중간에 return하면 안 됩니다!
    // ----------------------------------------------------

    bool isAlive = false;     // 살았는지 여부
    Particle p = (Particle)0; // 임시 초기화

    // 유효 범위 및 파티클 읽기
    if (dtID.x < readCount[emitterID])
    {
        p = readParticles[readParticleOffset + dtID.x];
        float dt = frameConsts[emitterID].dt;
        p.life -= dt;

        // 아직 살아있다면 업데이트 로직 수행
        if (p.life > 0.0f)
        {
            ForceConsts force = consts[emitterID].force;

            // 1. 물리 연산
            p.velocity += force.gravity * dt;
            p.velocity *= 1.0f / (1.0f + force.drag * dt);

            // Vortex
            VortexConsts vortex = consts[emitterID].vortex;
            if (vortex.active) {
                CalculateVortex(p, vortex, dt);
            }

            // Orbit
            OrbitConsts orbit = consts[emitterID].orbit;
            if (orbit.active) {
                CalculateOrbit(p, orbit, dt);
            }

            // 위치 갱신
            p.position += p.velocity * dt;

            // 2. 시각 효과
            VisualConsts visual = consts[emitterID].visual;
            float ageRatio = 1.0f - (p.life / max(p.lifeMax, 0.0001f));
            p.size = lerp(visual.sizeRange.x, visual.sizeRange.y, ageRatio);
            p.color = lerp(visual.startColor, visual.endColor, ageRatio);
            p.rotation = fmod(p.rotation + p.rotSpeed * dt, 6.28318530718f);

            // 생존 플래그 On
            isAlive = true;
        }
    }

    // ----------------------------------------------------
    // [3] LDS 최적화 (Counting & Reservation)
    // ----------------------------------------------------

    uint localIndex = 0; // 그룹 내에서의 내 순번 (0 ~ 1023)

    // A. 그룹 내 카운팅 (매우 빠름, 전역 병목 없음)
    if (isAlive)
    {
        InterlockedAdd(g_GroupAliveCount, 1, localIndex);
    }
    GroupMemoryBarrierWithGroupSync(); // 그룹 집계 끝날 때까지 대기

    // B. 전역 공간 예약 (그룹 대표 1명만 수행 -> 병목 1/1024로 감소)
    if (gtID.x == 0 && g_GroupAliveCount > 0)
    {
        // "우리 그룹 총 N명 살았으니, 전역 카운터에서 N개만큼 자리 주세요"
        InterlockedAdd(writeCount[emitterID], g_GroupAliveCount, g_GlobalBaseIndex);
    }
    GroupMemoryBarrierWithGroupSync(); // 전역 오프셋 받아올 때까지 대기

    // C. 최종 쓰기
    if (isAlive)
    {
        // 최종 인덱스 = (그룹이 받은 시작 번호) + (내 그룹 내 순번)
        uint finalIndex = g_GlobalBaseIndex + localIndex;

        // 결과 저장
        writeParticles[writeParticleOffset + finalIndex] = p;
    }
}