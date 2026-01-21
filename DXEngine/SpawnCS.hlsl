#include "ParticleCommon.hlsli"

AppendStructuredBuffer<Particle> outputParticles : register(u0);
StructuredBuffer<float3> meshVertexPositions : register(t0);
StructuredBuffer<uint> meshIndices : register(t1);

// --- [Robust Random Functions (Wang Hash)] ---

uint wang_hash(uint seed)
{
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}

float rand_float(inout uint state)
{
    state = wang_hash(state);
    return float(state) / 4294967296.0;
}

float rand_signed(inout uint state)
{
    return rand_float(state) * 2.0f - 1.0f;
}

// --- [Spawn Functions] ---

float3 BoxSpawn(inout uint rngState, float3 volume, float innerRatio) {
    float3 pos;
    pos.x = rand_signed(rngState);
    pos.y = rand_signed(rngState);
    pos.z = rand_signed(rngState);

    float3 posAbs = abs(pos);
    float3 hollowScale = lerp(innerRatio, 1.0f, posAbs);

    return sign(pos) * hollowScale * volume;
}

float3 SphereSpawn(inout uint rngState, float3 volume, float innerRatio) {
    // 균일 구면 분포
    float theta = rand_float(rngState) * 6.28318530718f; // 2 * PI
    float z = rand_float(rngState) * 2.0f - 1.0f;
    float r = sqrt(max(0.0f, 1.0f - z * z));

    float3 dir = float3(r * cos(theta), r * sin(theta), z);

    // 거리 랜덤 (Hollow)
    float dist = lerp(innerRatio, 1.0f, rand_float(rngState));

    return dir * dist * volume;
}

[numthreads(1024, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    if (dtID.x >= spawnCount)
        return;

    // 정수형 시드 초기화
    uint rngState = dtID.x * 1973 + uint(time * 10000.0f);

    // 초기 워밍업 (수정)
    rngState = wang_hash(rngState);

    Particle p;

    // [위치 결정 로직]
    if (spawn.spawnShape == 0) // BOX
    {
        p.position = BoxSpawn(rngState, spawn.spawnVolume, spawn.spawnInnerRatio);
    }
    else if (spawn.spawnShape == 1) // SPHERE
    {
        p.position = SphereSpawn(rngState, spawn.spawnVolume, spawn.spawnInnerRatio);
    }
    else if (spawn.spawnShape == 2) // Vertex
    {
        uint vCount = spawn.vertexCount;
        if (vCount > 0)
        {
            rngState = wang_hash(rngState); // 수정
            uint vIdx = rngState % vCount;
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

            rngState = wang_hash(rngState);
            uint triIdx = rngState % triCount;

            uint i0 = meshIndices[triIdx * 3 + 0];
            uint i1 = meshIndices[triIdx * 3 + 1];
            uint i2 = meshIndices[triIdx * 3 + 2];

            float3 p0 = meshVertexPositions[i0];
            float3 p1 = meshVertexPositions[i1];
            float3 p2 = meshVertexPositions[i2];

            float ra = rand_float(rngState);
            float rb = rand_float(rngState);

            if (ra + rb > 1.0f)
            {
                ra = 1.0f - ra;
                rb = 1.0f - rb;
            }

            p.position = p0 + ra * (p1 - p0) + rb * (p2 - p0);
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

    // Life
    p.life = lerp(spawn.lifeRange.x, spawn.lifeRange.y, rand_float(rngState));
    p.lifeMax = p.life;

    // Velocity
    float3 noiseDir;
    noiseDir.x = rand_signed(rngState);
    noiseDir.y = rand_signed(rngState);
    noiseDir.z = rand_signed(rngState);

    float3 finalDir = normalize(force.velocity + noiseDir * force.randomDir + 1e-5f);
    float speed = lerp(force.speedRange.x, force.speedRange.y, rand_float(rngState));

    p.velocity = finalDir * speed;

    // Color & Size
    p.color = visual.startColor;
    p.size = visual.sizeRange.x;

    float toRad = 3.141592f / 180.f;

    // Rotation
    float3 rndRot = float3(rand_float(rngState), rand_float(rngState), rand_float(rngState));
    p.rotation = lerp(visual.minRotation, visual.maxRotation, rndRot) * toRad;

    // RotSpeed
    float3 rndRotSpd = float3(rand_float(rngState), rand_float(rngState), rand_float(rngState));
    p.rotSpeed = lerp(visual.minRotSpeed, visual.maxRotSpeed, rndRotSpd);

    outputParticles.Append(p);
}