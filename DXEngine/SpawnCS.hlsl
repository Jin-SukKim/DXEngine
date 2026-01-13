#include "ParticleCommon.hlsli"
#include "Particle.hlsli"

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

[numthreads(256, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    // 이번 프레임 생성 할당량을 넘으면 종료
    if (dtID.x >= spawnCount)
        return;

    // 랜덤 시드 생성 (인덱스 + 시간 조합)
    float2 seed = float2(float(dtID.x), time);

    // 랜덤 값 추출
    float3 rPos;
    rPos.x = rand_hash_signed(seed);
    rPos.y = rand_hash_signed(seed + float2(1, 1));
    rPos.z = rand_hash_signed(seed + float2(2, 2));

    Particle p;

    // 1. Position 초기화 (Box Shape 기준)
    // spawnVolume 범위 내 랜덤 위치
    p.position = rPos * spawnVolume;

    // 2. Life 초기화
    // lifeRange.x ~ lifeRange.y 사이 랜덤
    p.life = rand_range(seed + float2(3, 3), lifeRange.x, lifeRange.y);
    p.lifeMax = p.life;

    // 3. Velocity 초기화
    // 기본 velocity 방향에 randomDir 만큼의 노이즈를 섞음
    float3 noiseDir;
    noiseDir.x = rand_hash_signed(seed + float2(10, 0));
    noiseDir.y = rand_hash_signed(seed + float2(0, 10));
    noiseDir.z = rand_hash_signed(seed + float2(5, 5));

    float3 finalDir = normalize(velocity + noiseDir * randomDir);

    // 속력(Speed) 랜덤 설정
    float speed = rand_range(seed + float2(4, 4), speedRange.x, speedRange.y);

    p.velocity = finalDir * speed;

    // 4. Color & Size 초기화
    p.color = startColor.rgb;
    p.size = sizeRange.x; // Start Size

    float toRad = 3.141592f / 180.f;
    p.rotation = lerp(minRotation, maxRotation, rand3(seed + 4.0)) * toRad;
    p.rotSpeed = lerp(minRotSpeed, maxRotSpeed, rand3(seed + 5.0));

    outputParticles.Append(p);
}