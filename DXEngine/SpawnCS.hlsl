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

float rand_hash_improved(float2 uv)
{
    // 더 나은 분포를 위한 개선된 해시
    return frac(sin(dot(uv, float2(127.1, 311.7))) * 43758.5453123);
}

uint rand_int_range(float2 seed, uint maxVal)
{
    if (maxVal == 0) return 0;
    float r = rand_hash_improved(seed);
    return min(uint(r * float(maxVal)), maxVal - 1);
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
    float2 seed = float2(dtID.xy) + time; // time도 ConstantBuffer에 있다고 가정
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
            uint vIdx = uint(rand_hash(float2(seed.x, 4.0)) * float(vCount)) % vCount;
            p.position = meshVertexPositions[vIdx];
        }
        else
        {
            p.position = float3(0, 0, 0);
        }
    }
    else if (spawn.spawnShape == 3) // Surface
    {
        uint iCount = spawn.indexCount;

        if (iCount > 0)
        {
            uint triCount = iCount / 3;

            // [수정 핵심] seed.x(ID)만 쓰지 않고 seed(ID + Time) 전체를 사용하여 랜덤성 확보
            // seed.x 와 seed.y를 모두 섞어야 매 프레임 다른 삼각형이 선택됨
            float randomVal = rand_hash(seed + float2(123.0, 456.0));

            uint triIdx = uint(randomVal * float(triCount)) % triCount;

            uint i0 = meshIndices[triIdx * 3 + 0];
            uint i1 = meshIndices[triIdx * 3 + 1];
            uint i2 = meshIndices[triIdx * 3 + 2];

            float3 p0 = meshVertexPositions[i0];
            float3 p1 = meshVertexPositions[i1];
            float3 p2 = meshVertexPositions[i2];

            // 평행사변형 접기 (Parallelogram Fold) - Surface 균일 분포
            // 삼각형 선택과는 다른 Seed 오프셋을 사용하여 상관관계 제거
            float ra = rand_hash(seed + float2(13.0, 41.0));
            float rb = rand_hash(seed + float2(29.0, 73.0));

            if (ra + rb > 1.0f)
            {
                ra = 1.0f - ra;
                rb = 1.0f - rb;
            }

            float3 pos = p0 + ra * (p1 - p0) + rb * (p2 - p0);

            p.position = pos;
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