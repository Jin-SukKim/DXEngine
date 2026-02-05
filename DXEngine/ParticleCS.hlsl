#include "ParticleCommon.hlsli"

// =================================================================================
// [Helper Functions]
// =================================================================================

// [Orbit] 로드리게스 회전 공식 (Axis-Angle Rotation)
float3 RotateVector(float3 v, float3 axis, float angle)
{
    float s, c;
    sincos(angle, s, c);
    return v * c + cross(axis, v) * s + axis * dot(axis, v) * (1 - c);
}

// [Vortex] 소용돌이 힘 계산
float3 CalculateVortexForce(float3 pos, float3 axis, float pull, VortexConsts vortex)
{
    float3 fromCenter = pos - vortex.vortexCenter;

    // 회전축에 투영된 벡터를 제거 -> 회전 평면 벡터
    float3 projected = fromCenter - dot(fromCenter, axis) * axis;
    float dist = length(projected); // 중심과의 거리

    if (dist < 0.0001f) return float3(0, 0, 0);

    float3 dir = normalize(projected);
    float3 tangent = cross(axis, dir); // 회전 방향 (접선)

    // 거리 감쇠
    float falloff = 1.0f / (1.0f + dist * dist * vortex.vortexFalloff);

    // Tangent Force(회전) + Radial Force(구심력/원심력)
    return ((tangent * vortex.vortexStrength) - (dir * pull)) * falloff;
}

// =================================================================================
// [Main Kernel]
// =================================================================================

[numthreads(256, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    // 1. [Global Indexing] 절대 주소로 접근
    uint globalIndex = dtID.x;

    // (안전장치) 전체 버퍼 크기 체크
    if (globalIndex >= TOTAL_MAX_PARTICLES) return;

    // 2. 파티클 읽기
    Particle p = readParticles[globalIndex];

    // 3. [생존 체크] 죽은 파티클은 연산하지 않음
    if (p.life <= 0.0f) return;

    // 4. [Owner 조회]
    uint myOwner = p.ownerID;
    EmitterID info = emitterIDs[myOwner];

    // Delta Time 조회
    float dt = frameConsts[info.emitterID].dt;

    // 수명 감소
    p.life -= dt;

    // 이번 프레임에 죽었다면 저장하지 않고 종료 (Auto-Defragmentation)
    if (p.life <= 0.0f) return;

    // 공통 변수: 생존 비율
    float ageRatio = 1.0f - (p.life / max(p.lifeMax, 0.0001f));

    // ============================================================================
    // [1] 기본 물리 연산 (Force Module)
    // ============================================================================
    ForceConsts force = consts[info.emitterID].force;

    // 중력 및 저항 적용
    p.velocity += force.gravity * dt;
    p.velocity *= 1.0f / (1.0f + force.drag * dt);

    // ============================================================================
    // [2] Vortex (소용돌이) 연산 - [active 체크]
    // ============================================================================
    VortexConsts vortex = consts[info.emitterID].vortex;

    // ★ active 변수로 활성화 여부 확인
    if (vortex.active)
    {
        // 시간 흐름에 따라 Pull 힘을 보간 (Start -> End)
        float currentPull = lerp(vortex.vortexPull.x, vortex.vortexPull.y, ageRatio);
        float3 axis = normalize(vortex.vortexAxis);

        float3 vForce = CalculateVortexForce(p.position, axis, currentPull, vortex);
        p.velocity += vForce * dt;
    }

    // ============================================================================
    // [3] 위치 통합 (Integration)
    // ============================================================================
    p.position += p.velocity * dt;

    // ============================================================================
    // [4] Orbit (궤도 회전) 연산 - [active 체크]
    // ============================================================================
    OrbitConsts orbit = consts[info.emitterID].orbit;

    // ★ active 변수로 활성화 여부 확인
    if (orbit.active)
    {
        float rotationAngle = orbit.rotationRate * dt;
        float3 axis = normalize(orbit.axis);

        // 중심 기준 상대 좌표
        float3 relativePos = p.position - orbit.center;

        // 위치 회전
        float3 newRelativePos = RotateVector(relativePos, axis, rotationAngle);
        p.position = orbit.center + newRelativePos;

        // 속도 벡터도 함께 회전 (방향 유지)
        p.velocity = RotateVector(p.velocity, axis, rotationAngle);
    }

    // ============================================================================
    // [5] 시각 효과 (Visuals)
    // ============================================================================
    VisualConsts visual = consts[info.emitterID].visual;

    p.size = lerp(visual.sizeRange.x, visual.sizeRange.y, ageRatio);
    p.color = lerp(visual.startColor, visual.endColor, ageRatio);
    p.rotation = fmod(p.rotation + p.rotSpeed * dt, 6.28318530718f);

    // ============================================================================
    // [6] 결과 저장 (Global Compaction)
    // ============================================================================
    uint writeIndex;
    InterlockedAdd(writeCount[info.emitterID], 1, writeIndex);

    writeParticles[info.particleOffset + writeIndex] = p;
}