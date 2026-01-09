#include "ParticleCommon.hlsli"

AppendStructuredBuffer<Particle> outputParticles : register(u0);
Buffer<uint> activeCount : register(t0);

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
[numthreads(256, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    // 이번 프레임에 생성해야 할 개수(spawnCount)를 넘으면 생성 중단
    if (dtID.x >= spawnCount)
        return;
    
    float2 seed = float2(dtID.x, time);
    float r1 = rand_hash(seed);
    float r2 = rand_hash(seed + 1.f);
    float r3 = rand_hash(seed + 2.f);
    float r4 = rand_hash(seed + 3.f);

    Particle p;
    
    // Spawn Position
    p.position = (float3(r1, r2, r3) - 0.5f) * 2.f * spawnVolume;
    // velocityBase : 주 방향
    // velocityRange : 랜덤 확산
    float3 randomDir = (float3(r2, r3, r1) - 0.5f) * 2.f;
    randomDir = normalize(randomDir + 0.0001f);
    p.velocity = (velocityBase + randomDir * velocityRand) * velocity;
    
    // Life
    p.life = lifeTimeBase + (r4 * lifeTimeRand);
    p.lifeMax = p.life;

    // Visual
    p.color = startColor;
    p.size = minMaxSize[0];
    p.rotation = r1 * 6.28f; // [0, 360]도 랜덤
    p.rotSpeed = lerp(minMaxRotateSpeed[0], minMaxRotateSpeed[1], r2);

    outputParticles.Append(p);
}