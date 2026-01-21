#include "ParticleCommon.hlsli"
#include "Common.hlsli"

struct Vertex {
    float3 position;
    float3 normalModel;
    float2 texcoord;
    float3 tangentModel;
};

AppendStructuredBuffer<Particle> outputParticles : register(u0);
StructuredBuffer<Vertex> meshVertex : register(t0);
StructuredBuffer<uint> meshIndices : register(t1);
Texture2D spawnTexture : register(t2);

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

// --- [Helper Functions] ---

// 텍스처 조건 검사
bool CheckTextureCondition(float2 uv)
{
    if (spawn.useTexture == 0) return true;

    float4 color = spawnTexture.SampleLevel(linearClampSampler, uv, 0);

    float value = dot(color, spawn.channelMask);

    return value >= spawn.textureThreshold;
}

// --- [Spawn Functions] ---

float3 BoxSpawn(inout uint rngState, float3 volume, float innerRatio)
{
    float3 pos;
    pos.x = rand_signed(rngState);
    pos.y = rand_signed(rngState);
    pos.z = rand_signed(rngState);

    float3 posAbs = abs(pos);
    float3 hollowScale = lerp(innerRatio, 1.0f, posAbs);

    return sign(pos) * hollowScale * volume;
}

float3 SphereSpawn(inout uint rngState, float3 volume, float innerRatio)
{
    float theta = rand_float(rngState) * 6.28318530718f;
    float z = rand_float(rngState) * 2.0f - 1.0f;
    float r = sqrt(max(0.0f, 1.0f - z * z));

    float3 dir = float3(r * cos(theta), r * sin(theta), z);
    float dist = lerp(innerRatio, 1.0f, rand_float(rngState));

    return dir * dist * volume;
}

// Vertex 스폰: 위치와 UV를 모두 반환
void VertexSpawn(inout uint rngState, uint vCount, out float3 outPos, out float2 outUV)
{
    if (vCount > 0)
    {
        rngState = wang_hash(rngState);
        uint vIdx = rngState % vCount;

        Vertex v = meshVertex[vIdx];
        outPos = v.position;
        outUV = v.texcoord;
    }
    else
    {
        outPos = float3(0, 0, 0);
        outUV = float2(0, 0);
    }
}

// Surface 스폰: 위치와 UV를 모두 반환
void SurfaceSpawn(inout uint rngState, uint iCount, out float3 outPos, out float2 outUV)
{
    if (iCount > 0)
    {
        uint triCount = iCount / 3;
        rngState = wang_hash(rngState);
        uint triIdx = rngState % triCount;

        uint i0 = meshIndices[triIdx * 3 + 0];
        uint i1 = meshIndices[triIdx * 3 + 1];
        uint i2 = meshIndices[triIdx * 3 + 2];

        // Barycentric Coords
        float ra = rand_float(rngState);
        float rb = rand_float(rngState);

        if (ra + rb > 1.0f)
        {
            ra = 1.0f - ra;
            rb = 1.0f - rb;
        }

        Vertex v0 = meshVertex[i0];
        Vertex v1 = meshVertex[i1];
        Vertex v2 = meshVertex[i2];

        // Position Interpolation
        outPos = v0.position + ra * (v1.position - v0.position) + rb * (v2.position - v0.position);

        // UV Interpolation
        outUV = v0.texcoord + ra * (v1.texcoord - v0.texcoord) + rb * (v2.texcoord - v0.texcoord);
    }
    else
    {
        outPos = float3(0, 0, 0);
        outUV = float2(0, 0);
    }
}

[numthreads(1024, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    if (dtID.x >= spawnCount)
        return;

    // 시드 초기화
    uint rngState = dtID.x * 1973 + uint(time * 10000.0f);
    rngState = wang_hash(rngState); // 워밍업

    Particle p;
    float3 spawnPos = float3(0, 0, 0);
    bool validSpawn = false;

    // [Rejection Sampling Loop]
    // 텍스처 스폰이 켜져있으면 조건에 맞을 때까지 최대 10번 재시도
    // 텍스처 스폰이 꺼져있으면 1번만 수행
    int maxAttempts = (spawn.useTexture != 0 && spawn.spawnShape >= 2) ? 10 : 1;

    for (int i = 0; i < maxAttempts; ++i)
    {
        // 매 시도마다 시드 갱신
        rngState = wang_hash(rngState);
        float3 tempPos = float3(0, 0, 0);
        float2 tempUV = float2(0, 0);
        bool needsCheck = false;

        if (spawn.spawnShape == 0) // BOX
        {
            tempPos = BoxSpawn(rngState, spawn.spawnVolume, spawn.spawnInnerRatio);
            validSpawn = true;
        }
        else if (spawn.spawnShape == 1) // SPHERE
        {
            tempPos = SphereSpawn(rngState, spawn.spawnVolume, spawn.spawnInnerRatio);
            validSpawn = true;
        }
        else if (spawn.spawnShape == 2) // VERTEX
        {
            VertexSpawn(rngState, spawn.vertexCount, tempPos, tempUV);
            needsCheck = true;
        }
        else if (spawn.spawnShape == 3) // SURFACE
        {
            SurfaceSpawn(rngState, spawn.indexCount, tempPos, tempUV);
            needsCheck = true;
        }

        // 텍스처 조건 검사 (Mesh 타입일 경우에만)
        if (needsCheck)
        {
            if (CheckTextureCondition(tempUV))
            {
                spawnPos = tempPos;
                validSpawn = true;
                break; // 성공! 루프 탈출
            }
            // 실패하면 continue (다음 시도)
        }
        else
        {
            spawnPos = tempPos;
            break; // Mesh 타입이 아니면 바로 성공
        }
    }

    // 조건 만족 실패 시 (너무 어두운 부분 등) 파티클을 생성하지 않음
    if (!validSpawn) return;

    // 성공한 위치 적용
    p.position = spawnPos + spawn.localPos;

    // --- Life, Velocity, etc. ---

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