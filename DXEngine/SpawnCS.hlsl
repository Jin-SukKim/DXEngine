#include "ParticleCommon.hlsli"

AppendStructuredBuffer<Particle> outputParticles : register(u0);

float rand_hash_signed(float2 uv)
{
    // 0~1 범위의 해시를 구한 뒤 -1~1로 변환
    float h = frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
    return h * 2.0f - 1.0f;
}

// 0.0 ~ 1.0 사이의 값을 반환
float rand_hash(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

// [min, max] random
float rand_range(float2 seed, float minVal, float maxVal)
{
    return minVal + (maxVal - minVal) * rand_hash(seed);
}

float3 rand3(float2 seed) {
    return float3(
        frac(sin(dot(seed, float2(12.9898, 78.233))) * 43758.5453),
        frac(sin(dot(seed, float2(39.346, 11.135))) * 43758.5453),
        frac(sin(dot(seed, float2(73.156, 52.235))) * 43758.5453)
        );
}

[numthreads(1024, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    if (dtID.x >= spawnCount)
        return;

    // 1. 기본 시드 생성
    float2 seed = float2(float(dtID.x), time);
    Particle p;

    // [수정] 모양 결정 로직 전면 개편
    // 0~1 사이의 독립적인 랜덤값 3개를 먼저 뽑습니다.
    float3 r01 = rand3(seed);

    // -1~1 사이의 랜덤값 (방향성 결정용)
    float3 rSigned = r01 * 2.0f - 1.0f;

    if (spawn.spawnShape == 0) // BOX
    {
        // [Box 수정] 
        // 기존 코드의 문제: rPosAbs 변수가 중복 선언되어 있었고, 로직이 꼬여있었음.

        float3 pos = rSigned; // -1 ~ 1
        float3 posAbs = abs(pos);

        // 안쪽 비우기 (Hollow)
        // 0~1 사이인 posAbs를 inner~1 사이로 매핑 -> 다시 부호 적용
        float3 hollowScale = lerp(spawn.spawnInnerRatio, 1.0f, posAbs);

        // 최종 위치: 부호 * (안쪽비율적용된크기) * 전체볼륨
        p.position = sign(pos) * hollowScale * spawn.spawnVolume;
    }
    else // SPHERE
    {
        // [Sphere 수정]
        // 핵심: 방향(Direction)과 거리(Distance)는 '완전히 다른 시드' 혹은 '다른 변수'를 써야 함.

        // 1. 방향 구하기 (rSigned 사용)
        float3 dir = normalize(rSigned);

        // 예외 처리: 랜덤 벡터 길이가 너무 짧으면 위쪽을 보게 함
        if (length(rSigned) < 0.001f) dir = float3(0, 1, 0);

        // 2. 거리 구하기 (독립적인 랜덤값 필요!)
        // 아까 뽑은 r01을 다 썼으므로, 새로운 시드로 랜덤 하나를 더 뽑습니다.
        float randDist = rand_hash(seed + float2(123.45, 678.90)); // 시드 오프셋

        // 3. 거리 보간 (Hollow 적용)
        // 구의 중심(0) ~ 표면(1) 사이에서 InnerRatio ~ 1.0 사이로 보간
        float distScale = lerp(spawn.spawnInnerRatio, 1.0f, randDist);

        // 4. 최종 위치 적용
        // dir(방향) * distScale(거리 0~1) * spawnVolume(반지름)
        p.position = dir * distScale * spawn.spawnVolume;
    }

    p.position += spawn.localPos;

    // --- 아래는 기존 유지 (일부 시드 오프셋만 충돌 안 나게 조정) ---

    // 2. Life
    p.life = rand_range(seed + float2(3, 3), spawn.lifeRange.x, spawn.lifeRange.y);
    p.lifeMax = p.life;

    // 3. Velocity
    float3 noiseDir;
    // 시드 오프셋을 겹치지 않게 확실히 분리
    noiseDir.x = rand_hash_signed(seed + float2(10.1, 0.5));
    noiseDir.y = rand_hash_signed(seed + float2(0.5, 10.2));
    noiseDir.z = rand_hash_signed(seed + float2(5.5, 5.5));

    float3 finalDir = normalize(force.velocity + noiseDir * force.randomDir);
    float speed = rand_range(seed + float2(4, 4), force.speedRange.x, force.speedRange.y);

    p.velocity = finalDir * speed;

    // 4. Color & Size
    p.color = visual.startColor;
    p.size = visual.sizeRange.x;

    float toRad = 3.141592f / 180.f;
    p.rotation = lerp(visual.minRotation, visual.maxRotation, rand3(seed + 7.0)) * toRad; // 시드 오프셋 변경
    p.rotSpeed = lerp(visual.minRotSpeed, visual.maxRotSpeed, rand3(seed + 8.0)); // 시드 오프셋 변경

    outputParticles.Append(p);
}