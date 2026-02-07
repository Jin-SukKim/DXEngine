#include "ParticleCommon.hlsli"
#include "OrbitCS.hlsli"
#include "VortexCS.hlsli"

[numthreads(1024, 1, 1)]
void main(uint3 gID : SV_GroupID, int3 gtID : SV_GroupThreadID, uint3 dtID : SV_DispatchThreadID)
{
    Particle p = readParticles[dtID.x];
    
    EmitterID eID = emitterIDs[p.ownerID];
    if (dtID.x < eID.readParticleOffset || dtID.x >= eID.readParticleOffset + readCount[p.ownerID])
        return;
    
    float dt = frameConsts[p.ownerID].dt;
    p.life -= dt;

    if (p.life <= 0.f)
        return;

    ForceConsts force = consts[p.ownerID].force;
    // 1. 물리 연산 (Physics)

    // 중력 적용
    p.velocity += force.gravity * dt;

    // 공기 저항 (Drag) 적용
    // drag 값이 클수록 속도가 0에 빠르게 수렴
    p.velocity *= 1.0f / (1.0f + force.drag * dt);

    // Vortex
    VortexConsts vortex = consts[p.ownerID].vortex;
    if (vortex.active)
    {
        CalculateVortex(p, vortex, dt);
    }

    // Orbit
    OrbitConsts orbit = consts[p.ownerID].orbit;
    if (orbit.active)
    {
        CalculateOrbit(p, orbit, dt);
    }

    // 위치 갱신
    p.position += p.velocity * dt;

    // 2. 시각 효과 (Visuals)
    VisualConsts visual = consts[p.ownerID].visual;
    float ageRatio = 1.0f - (p.life / max(p.lifeMax, 0.0001f));
    // 크기 보간 (Start -> End)
    // sizeRange.x = Start Size, sizeRange.y = End Size
    p.size = lerp(visual.sizeRange.x, visual.sizeRange.y, ageRatio);

    // 색상 보간 (Start -> End)
    p.color = lerp(visual.startColor, visual.endColor, ageRatio);

    //p.rotation += p.rotSpeed * dt;
    p.rotation = fmod(p.rotation + p.rotSpeed * dt, 6.28318530718f);

    // 결과 저장
    uint writeIndex;
    InterlockedAdd(writeCount[p.ownerID], 1, writeIndex);
    writeParticles[eID.writeParticleOffset + writeIndex] = p;
}