#include "ParticleCommon.hlsli"

// 출력 버퍼
AppendStructuredBuffer<Particle> outputParticles : register(u0);

// 메쉬 데이터 버퍼
StructuredBuffer<float3> meshVertexPositions : register(t0);
StructuredBuffer<uint> meshIndices : register(t1);

// --- [Helper Functions] ---

float rand_hash_signed(float2 uv)
{
    float h = frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
    return h * 2.0f - 1.0f;
}

float rand_hash(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

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

// --- [Spawn Functions] ---
// 수정: spawn 데이터와 seed를 매개변수로 받아야 함수 내부에서 사용 가능합니다.

float3 BoxSpawn(float3 rSigned, float3 volume, float innerRatio) {
    float3 pos = rSigned; // -1 ~ 1
    float3 posAbs = abs(pos);

    // 안쪽 비우기 (Hollow)
    // 0~1 사이인 posAbs를 inner~1 사이로 매핑
    float3 hollowScale = lerp(innerRatio, 1.0f, posAbs);

    // 최종 위치: 부호 * (안쪽비율적용된크기) * 전체볼륨
    return sign(pos) * hollowScale * volume;
}

float3 SphereSpawn(float3 rSigned, float2 seed, float3 volume, float innerRatio) {
    // 1. 방향 구하기
    float3 dir = normalize(rSigned);
    if (length(rSigned) < 0.001f) dir = float3(0, 1, 0);

    // 2. 거리 구하기
    // seed를 받아와서 새로운 랜덤값을 생성
    float randDist = rand_hash(seed + float2(123.45, 678.90));

    // 3. 거리 보간 (Hollow 적용)
    // 주의: 구의 부피는 반지름의 3제곱에 비례하므로, 균일한 분포를 위해 세제곱근을 쓰기도 하지만,
    // 간단한 구현에서는 선형 보간도 허용됩니다. (여기선 선형 보간 유지)
    float distScale = lerp(innerRatio, 1.0f, randDist);

    // 4. 최종 위치 적용
    // volume이 float3(반지름)라고 가정하고 component-wise 곱셈 적용
    return dir * distScale * volume;
}

[numthreads(1024, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    if (dtID.x >= spawnCount) // spawnCount는 ConstantBuffer(ParticleCommon.hlsli)에 있다고 가정
        return;

    // 1. 기본 시드 생성
    float2 seed = float2(float(dtID.x), time); // time도 ConstantBuffer에 있다고 가정
    Particle p;

    // 0~1 사이 랜덤 3개
    float3 r01 = rand3(seed);
    // -1~1 사이 랜덤 3개 (방향성 결정용)
    float3 rSigned = r01 * 2.0f - 1.0f;

    // [위치 결정 로직]
    if (spawn.spawnShape == 0) // BOX
    {
        // 수정: 매개변수 전달
        p.position = BoxSpawn(rSigned, spawn.spawnVolume, spawn.spawnInnerRatio);
    }
    else if (spawn.spawnShape == 1) // SPHERE
    {
        // 수정: 매개변수 전달 (seed 포함)
        p.position = SphereSpawn(rSigned, seed, spawn.spawnVolume, spawn.spawnInnerRatio);
    }
    else if (spawn.spawnShape == 2) // Vertex
    {
        // 수정: rand() -> rand_hash()
        // vertexCount가 ConstantBuffer에 없다면 meshVertexPositions.GetDimensions() 등으로 가져와야 함
        uint vCount = spawn.vertexCount;
        if (vCount > 0)
        {
            uint vIdx = uint(rand_hash(float2(seed.x, 4.0)) * vCount) % vCount;
            p.position = meshVertexPositions[vIdx];
        }
        else
        {
            p.position = float3(0, 0, 0);
        }
    }
    else if (spawn.spawnShape == 3) // Surface
    {
        // 수정: 변수명 일치 (g_IndexCount -> spawn.indexCount 혹은 계산)
        uint iCount = spawn.indexCount; // ConstantBuffer에 있다고 가정

        if (iCount > 0)
        {
            uint triCount = iCount / 3;
            // 수정: rand() -> rand_hash(), 변수명 g_MeshIndices -> meshIndices
            uint triIdx = uint(rand_hash(float2(seed.x, 5.0)) * triCount) % triCount;

            uint i0 = meshIndices[triIdx * 3 + 0];
            uint i1 = meshIndices[triIdx * 3 + 1];
            uint i2 = meshIndices[triIdx * 3 + 2];

            // 수정: g_MeshPositions -> meshVertexPositions
            float3 p0 = meshVertexPositions[i0];
            float3 p1 = meshVertexPositions[i1];
            float3 p2 = meshVertexPositions[i2];

            // 무게중심 좌표 계산
            float u = rand_hash(float2(seed.x, 6.0)); // seed.x, y 혼용 주의 (여기선 seed 전체가 float2라 가정)
            float v = rand_hash(float2(seed.y, 7.0));

            if (u + v > 1.0)
            {
                u = 1.0 - u;
                v = 1.0 - v;
            }
            p.position = p0 + u * (p1 - p0) + v * (p2 - p0);
        }
        else
        {
            p.position = float3(0, 0, 0);
        }
    }
    else
    {
        p.position = float3(0, 0, 0);
    }

    p.position += spawn.localPos;

    // --- Life, Velocity, etc. (기존 코드 유지 및 rand_hash 통일) ---

    // 2. Life
    p.life = rand_range(seed + float2(3, 3), spawn.lifeRange.x, spawn.lifeRange.y);
    p.lifeMax = p.life;

    // 3. Velocity
    float3 noiseDir;
    noiseDir.x = rand_hash_signed(seed + float2(10.1, 0.5));
    noiseDir.y = rand_hash_signed(seed + float2(0.5, 10.2));
    noiseDir.z = rand_hash_signed(seed + float2(5.5, 5.5));

    float3 finalDir = normalize(force.velocity + noiseDir * force.randomDir + 1e-10f);
    float speed = rand_range(seed + float2(4, 4), force.speedRange.x, force.speedRange.y);

    p.velocity = finalDir * speed;

    // 4. Color & Size
    p.color = visual.startColor;
    p.size = visual.sizeRange.x;

    float toRad = 3.141592f / 180.f;
    // 수정: rand3 결과는 float3이므로 rotation이 float3인지 float인지에 따라 수정 필요.
    // 여기서는 float3 Rotation (Euler)라고 가정합니다.
    p.rotation = lerp(visual.minRotation, visual.maxRotation, rand3(seed + 7.0)) * toRad;
    p.rotSpeed = lerp(visual.minRotSpeed, visual.maxRotSpeed, rand3(seed + 8.0));

    outputParticles.Append(p);
}