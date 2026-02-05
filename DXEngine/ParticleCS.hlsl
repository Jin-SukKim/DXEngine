#include "ParticleCommon.hlsli"

float3 RotateVector(float3 v, float3 axis, float angle)
{
    float s, c;
    sincos(angle, s, c);
    return v * c + cross(axis, v) * s + axis * dot(axis, v) * (1 - c);
}

float3 CalculateVortexForce(float3 pos, float3 axis, float pull, VortexConsts vortex)
{
    float3 fromCenter = pos - vortex.vortexCenter;
    float3 projected = fromCenter - dot(fromCenter, axis) * axis;
    float dist = length(projected);

    if (dist < 0.0001f)
        return float3(0, 0, 0);

    float3 dir = normalize(projected);
    float3 tangent = cross(axis, dir);
    float falloff = 1.0f / (1.0f + dist * dist * vortex.vortexFalloff);

    return ((tangent * vortex.vortexStrength) - (dir * pull)) * falloff;
}

[numthreads(256, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint globalIndex = dtID.x;

    if (globalIndex >= TOTAL_MAX_PARTICLES)
        return;

    Particle p = readParticles[globalIndex];

    if (p.life <= 0.0f)
        return;

    // ★ ownerID는 글로벌 슬롯 인덱스
    uint globalEmitterSlot = p.ownerID;
    
    // ★ EmitterID 정보 가져오기
    EmitterID info = emitterIDs[globalEmitterSlot];
    
    // ★ consts와 frameConsts는 글로벌 슬롯 인덱스로 접근
    // (emitterIDs[x].emitterID == x 이므로 info.emitterID나 globalEmitterSlot이나 같음)
    ParticleConsts c = consts[globalEmitterSlot];
    float dt = frameConsts[globalEmitterSlot].dt;

    p.life -= dt;
    if (p.life <= 0.0f)
        return;

    float ageRatio = 1.0f - (p.life / max(p.lifeMax, 0.0001f));

    // ============================================================================
    // [1] Force Module (항상 적용)
    // ============================================================================
    ForceConsts force = c.force;
    p.velocity += force.gravity * dt;
    p.velocity *= 1.0f / (1.0f + force.drag * dt);

    // ============================================================================
    // [2] Vortex Module - ★ active 체크
    // ============================================================================
    VortexConsts vortex = c.vortex;
    if (vortex.active)
    {
        float3 axis = vortex.vortexAxis;
        float axisLen = length(axis);
    
    // ★ axis가 유효한 경우에만 처리
        if (axisLen > 0.0001f)
        {
            axis = axis / axisLen;
            float currentPull = lerp(vortex.vortexPull.x, vortex.vortexPull.y, ageRatio);
            float3 vForce = CalculateVortexForce(p.position, axis, currentPull, vortex);
            p.velocity += vForce * dt;
        }
    }

    // ============================================================================
    // [3] Position Update
    // ============================================================================
    p.position += p.velocity * dt;

    // ============================================================================
    // [4] Orbit Module - ★ active 체크
    // ============================================================================
    OrbitConsts orbit = c.orbit;
    if (orbit.active)
    {
        float rotationAngle = orbit.rotationRate * dt;
        float3 orbitAxis = orbit.axis;
        float axisLen = length(orbitAxis);
    
    // ★ axis가 유효한 경우에만 처리
        if (axisLen > 0.0001f)
        {
            orbitAxis = orbitAxis / axisLen; // normalize
            float3 relativePos = p.position - orbit.center;
            float3 newRelativePos = RotateVector(relativePos, orbitAxis, rotationAngle);
            p.position = orbit.center + newRelativePos;
            p.velocity = RotateVector(p.velocity, orbitAxis, rotationAngle);
        }
    }

    // ============================================================================
    // [5] Visual Update
    // ============================================================================
    VisualConsts visual = c.visual;
    p.size = lerp(visual.sizeRange.x, visual.sizeRange.y, ageRatio);
    p.color = lerp(visual.startColor, visual.endColor, ageRatio);
    p.rotation = fmod(p.rotation + p.rotSpeed * dt, 6.28318530718f);

    // ============================================================================
    // [6] Write to buffer
    // ============================================================================
    uint writeIndex;
    InterlockedAdd(writeCount[globalEmitterSlot], 1, writeIndex);

    uint maxParticles = frameConsts[globalEmitterSlot].maxParticles;
    if (writeIndex < maxParticles)
    {
        // ★ writeParticleOffset 사용
        writeParticles[info.writeParticleOffset + writeIndex] = p;
    }
}