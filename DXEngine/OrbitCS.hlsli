// �ε帮�Խ� ȸ�� ���� (Axis-Angle Rotation)
float3 RotateVector(float3 v, float3 axis, float angle)
{
    float s, c;
    sincos(angle, s, c);
    return v * c + cross(axis, v) * s + axis * dot(axis, v) * (1 - c);
}

void CalculateOrbit(inout Particle p, OrbitConsts orbit, float dt)
{
    // 1. ȸ���� ���� ��� (Rate * DeltaTime)
    float ageRatio = 1.0f - (p.life / max(p.lifeMax, 0.0001f));
    float orbitCurve = SampleCurve(CURVE_ORBIT_RATE, ageRatio, emitterIDs[p.ownerID].curveLUTSlice);
    float rotationAngle = orbit.rotationRate * orbitCurve * dt;

    // 2. �߽� ���� ��� ��ǥ ���ϱ�
    float3 relativePos = p.position - orbit.center;

    // 3. ����ȭ�� ���� �������� ��ġ ���� ȸ��
    float3 axis = normalize(orbit.axis);
    float3 newRelativePos = RotateVector(relativePos, axis, rotationAngle);

    // 4. ���ο� ��ġ ����
    p.position = orbit.center + newRelativePos;

    // 5. �ӵ� ���͵� �Բ� ȸ�� (�߿�!)
    p.velocity = RotateVector(p.velocity, axis, rotationAngle);
}