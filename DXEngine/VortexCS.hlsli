float3 CalculateVortexForce(float3 pos, float3 axis, float pull, uint ownerID)
{
    VortexConsts vortex = consts[ownerID].vortex;
    float3 fromCenter = pos - vortex.vortexCenter;

    // 회전축에 투영된 벡터를 제거 -> 회전 평면 벡터 (회전축에 수직인 벡터)
    float3 projected = fromCenter - dot(fromCenter, axis) * axis;
    float dist = length(projected); // 중심과의 거리

    if (dist < 0.0001)
        return float3(0, 0, 0);

    float3 dir = normalize(projected);
    float3 tangent = cross(axis, dir); // 회전 방향

    // 거리 감쇠: 1/(1+dist²) - 중심에서 멀어질수록 힘이 약해짐
    float falloff = 1.0f / (1.0f + dist * dist * vortex.vortexFalloff);

    // Tangent Force(회전) + Radial Force(구심력/원심력)
    return ((tangent * vortex.vortexStrength) - (dir * pull)) * falloff;
}

void CalculateVortex(inout Particle p, VortexConsts vortex, float dt)
{
    // 생존 비율 (0.0: 탄생 직후 ~ 1.0: 사망 직전)
    float ageRatio = 1.0f - (p.life / max(p.lifeMax, 0.0001f));

    // 시간 흐름에 따라 Pull 힘을 보간 (Start -> End)
    float currentPull = lerp(vortex.vortexPull[0], vortex.vortexPull[1], ageRatio);

    if (abs(vortex.vortexStrength) > 0.001 || abs(currentPull) > 0.001)
    {
        float3 normalizedAxis = normalize(vortex.vortexAxis);
        float3 vForce = CalculateVortexForce(p.position, normalizedAxis, currentPull, p.ownerID);
        p.velocity += vForce * dt;
    }
}