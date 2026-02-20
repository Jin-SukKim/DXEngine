// 로드리게스 회전 공식 (Axis-Angle Rotation)
float3 RotateVector(float3 v, float3 axis, float angle)
{
    float s, c;
    sincos(angle, s, c);
    return v * c + cross(axis, v) * s + axis * dot(axis, v) * (1 - c);
}

void CalculateOrbit(inout Particle p, OrbitConsts orbit, float dt, float ageRatio)
{
    // 1. 회전할 각도 계산 (Rate * DeltaTime)
    float orbitCurve = SampleCurve(CURVE_ORBIT_RATE, ageRatio, emitterIDs[p.ownerID].curveLUTSlice);
    float rotationAngle = orbit.rotationRate * orbitCurve * dt;

    // 2. 중심 기준 상대 좌표 구하기
    float3 relativePos = p.position - orbit.center;

    // 3. 정규화된 축을 기준으로 위치 벡터 회전
    float3 axis = normalize(orbit.axis);
    float3 newRelativePos = RotateVector(relativePos, axis, rotationAngle);

    // 4. 새로운 위치 적용
    p.position = orbit.center + newRelativePos;

    // 5. 속도 벡터도 함께 회전 (중요!)
    p.velocity = RotateVector(p.velocity, axis, rotationAngle);
}