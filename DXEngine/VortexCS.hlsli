float3 CalculateVortexForce(float3 pos, float3 axis, float pull, uint ownerID)
{
    VortexConsts vortex = consts[ownerID].vortex;
    float3 fromCenter = pos - vortex.vortexCenter;

    // ȸ���࿡ ������ ���͸� ���� -> ȸ�� ��� ���� (ȸ���࿡ ������ ����)
    float3 projected = fromCenter - dot(fromCenter, axis) * axis;
    float dist = length(projected); // �߽ɰ��� �Ÿ�

    if (dist < 0.0001)
        return float3(0, 0, 0);

    float3 dir = normalize(projected);
    float3 tangent = cross(axis, dir); // ȸ�� ����

    // �Ÿ� ����: 1/(1+dist��) - �߽ɿ��� �־������� ���� ������
    float falloff = 1.0f / (1.0f + dist * dist * vortex.vortexFalloff);

    // Tangent Force(ȸ��) + Radial Force(���ɷ�/���ɷ�)
    return ((tangent * vortex.vortexStrength) - (dir * pull)) * falloff;
}

void CalculateVortex(inout Particle p, VortexConsts vortex, float dt)
{
    // ���� ���� (0.0: ź�� ���� ~ 1.0: ��� ����)
    float ageRatio = 1.0f - (p.life / max(p.lifeMax, 0.0001f));

    // �ð� �帧�� ���� Pull ���� ���� (Start -> End)
    float currentPull = lerp(vortex.vortexPull[0], vortex.vortexPull[1], ageRatio);

    float vortexCurve = SampleCurve(CURVE_VORTEX_STRENGTH, ageRatio, emitterIDs[p.ownerID].curveLUTSlice);

    if (abs(vortex.vortexStrength) > 0.001 || abs(currentPull) > 0.001)
    {
        float3 normalizedAxis = normalize(vortex.vortexAxis);
        float3 vForce = CalculateVortexForce(p.position, normalizedAxis, currentPull, p.ownerID);
        p.velocity += vForce * vortexCurve * dt;
    }
}