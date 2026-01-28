#include "ParticleCommon.hlsli"

Buffer<uint> activeCount : register(t0);
RWStructuredBuffer<Particle> inputParticles : register(u0);

// 로드리게스 회전 공식 (Axis-Angle Rotation)
float3 RotateVector(float3 v, float3 axis, float angle)
{
    float s, c;
    sincos(angle, s, c);
    return v * c + cross(axis, v) * s + axis * dot(axis, v) * (1 - c);
}

[numthreads(1024, 1, 1)]
void main(uint3 gID : SV_GroupID, int3 gtID : SV_GroupThreadID, uint3 dtID : SV_DispatchThreadID)
{
    if (dtID.x >= activeCount[0])
        return;

    Particle p = inputParticles[dtID.x];

    // 1. 회전할 각도 계산 (Rate * DeltaTime)
    // 매 프레임 조금씩 돌립니다.
    float rotationAngle = orbit.rotationRate * dt;

    // 2. 중심 기준 상대 좌표 구하기
    float3 relativePos = p.position - orbit.center;

    // (옵션) 강제 오프셋 적용: 만약 거리를 강제하고 싶다면 아래 주석 해제
    // float currentDist = length(relativePos);
    // if (orbit.offset > 0.0f && currentDist > 0.001f) {
    //     relativePos = normalize(relativePos) * orbit.offset;
    // }

    // 3. 정규화된 축을 기준으로 위치 벡터 회전
    float3 axis = normalize(orbit.axis);
    float3 newRelativePos = RotateVector(relativePos, axis, rotationAngle);

    // 4. 새로운 위치 적용
    p.position = orbit.center + newRelativePos;

    // 5. 속도 벡터도 함께 회전 (중요!)
    // 속도를 회전시키지 않으면, 입자가 이동하던 방향(관성)과 궤도 회전이 어긋나서
    // 밖으로 튀어 나가거나 이상한 나선형을 그리게 됩니다.
    p.velocity = RotateVector(p.velocity, axis, rotationAngle);

    inputParticles[dtID.x] = p;
}