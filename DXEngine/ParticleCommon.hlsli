#ifndef __PARTICLE_COMMON_HLSLI__
#define __PARTICLE_COMMON_HLSLI__

struct Particle
{
    float3 position;
    float3 velocity;
    float4 color;
    float life;
    float lifeMax;
    float size;
    float3 rotation;
    float3 rotSpeed;
    uint ownerID;
    uint systemID;
};

RWStructuredBuffer<Particle> particles : register(u5);           // Single buffer (in-place update)
RWStructuredBuffer<uint> writeAliveIndices : register(u6);       // Write alive indices (ping-pong)
RWStructuredBuffer<uint> writeAliveCount : register(u7);         // Write alive count per emitter

#ifdef PARTICLE_RENDER_STAGE
    // CB5 contains batch info during rendering
    cbuffer BatchInfo : register(b5) {
        uint batchEmitterCount;
        uint batchEmitterListOffset;
        uint batchInstanceOffset;
        uint batchInfoPadding;
    };
#else
    // CB5 contains emitterID during spawn/update (unchanged)
    cbuffer EmitterID : register(b5)
    {
        uint readParticleOffset;
        uint writeParticleOffset;
        uint emitterID;
        uint spawnPosOffset;  // bakedOffset + customOffset
        uint systemID;
        float3 paddingID;
        uint curveLUTSlice;
        float3 paddingID2;
    };
#endif

struct EmitterID
{
    uint readParticleOffset;
    uint writeParticleOffset;
    uint emitterID;
    uint spawnPosOffset;  // bakedOffset + customOffset 
    uint systemID;
    uint indexCount;
    uint startIndexLocation;
    uint baseVertexLocation;
    uint curveLUTSlice;
    float3 paddingID2;
};

struct ParticleFrameConsts
{
    float dt;
    float time;
    uint spawnCount;
    uint maxParticles;
    float spawnRatio;       // Current frame spawn ratio (0.0 ~ 1.0)
    float3 padding2;
};

struct SpawnConsts
{
    float3 localPos;
    float padding;

    float3 spawnVolume;
    float spawnInnerRatio;

    float2 lifeRange;
    int spawnShape;
    uint bakedCount; // Baked/Custom  
    uint simulationSpace;

    uint spawnStartIndex;
    float2 padding1;
};

struct VisualConsts
{
    float2 sizeRange;
    float2 padding2;

    float4 startColor;
    float4 endColor;

    float3 minRotation;
    float padding3;
    float3 maxRotation;
    float padding4;
    float3 minRotSpeed;
    float padding5;
    float3 maxRotSpeed;
    float padding6;

    // Overdraw Control - Size Scaling
    float sizeDistanceScale;      // Size multiplier at close range (e.g., 0.7 = 70%)
    float sizeDistanceMin;        // Distance where scaling starts (meters)
    float sizeDistanceMax;        // Distance where scaling ends (meters)
    uint enableSizeScaling;       // On/Off toggle (per-emitter)
};

struct ForceConsts
{
    float3 velocity;
    float padding7;
    float2 speedRange;
    float2 padding8;

    float3 randomDir;
    float drag;
    float3 gravity;
    float padding9;

    // Curl Noise (Tiling Mode)
    float curlNoiseFrequency;
    float curlNoiseStrength;
    uint curlNoiseEnabled;
    float padding10;
    float3 curlNoiseScrollSpeed;
    float paddingCurl;
};

struct RenderConsts
{
    uint frameCount;
    float2 frameTiles;
    uint indexCount;
    
    uint useSorting;
    float animDuration;     // NEW: 0.0~1.0 (Animation duration as ratio of particle lifetime)
    float softDistance;     // Soft particle fade distance (0 = disabled)
    uint frameBlending;
    
    float animTime;
    float velocityStretchFactor; // 0 = disabled
    uint uvDistortEnabled;
    float uvDistortFrequency;
    
    float uvDistortStrength;
    float2 uvDistortScroll;
    float renderPadding;
};

struct VortexConsts
{
    float vortexStrength;
    float3 vortexCenter;
    float3 vortexAxis;
    float vortexFalloff;
    float2 vortexPull;
    float padding10;
    uint active;
};

struct OrbitConsts
{
    float3 center;
    float rotationRate;
    float3 axis;
    float initialOffset;
    uint active;
    float3 orbitPadding;
};

struct ParticleConsts
{
    SpawnConsts spawn;
    VisualConsts visual;
    ForceConsts force;
    RenderConsts render;
    VortexConsts vortex;
    OrbitConsts orbit;
};

struct ParticleMeshConsts
{
    matrix pWorld;
    matrix pWorldIT;
    uint vertexCount;
    uint indexCount;
    uint vertexOffset;
    uint indexOffset;
};

struct BatchDescriptor {
    uint emitterCount;
    uint emitterListOffset;
    uint instanceOffset;
    uint indexCount;
    uint startIndexLocation;
    uint baseVertexLocation;
    uint isMesh;
    uint padding;
};
Texture2DArray particleTex : register(t14);

// Curl Noise 3D Texture
Texture3D curlNoiseTexture : register(t26);
Texture2D curlNoiseTexture2D : register(t27);
SamplerState curlNoiseSampler : register(s0);

// Curve LUT Texture Array 
Texture2DArray curveLUT : register(t28);
SamplerState curveSampler : register(s1);  // linearClampSS

// Curve type row indices (matches ParticleCurveType enum)
static const uint CURVE_COLOR = 0;
static const uint CURVE_ALPHA = 1;
static const uint CURVE_SIZE = 2;
static const uint CURVE_VELOCITY = 3;
static const uint CURVE_DRAG = 4;
static const uint CURVE_GRAVITY = 5;
static const uint CURVE_NOISE_STRENGTH = 6;
static const uint CURVE_VORTEX_STRENGTH = 7;
static const uint CURVE_ORBIT_RATE = 8;
static const uint CURVE_COUNT = 9;

float SampleCurve(uint curveType, float t, uint sliceIdx) {
    // TODO: ���� 0.5�� ������ �ʿ䰡 �ֳ�?
    float v = ((float)curveType + 0.5) / (float)CURVE_COUNT;
    return curveLUT.SampleLevel(curveSampler, float3(t, v, sliceIdx), 0).r;
}

StructuredBuffer<Particle> readParticles : register(t16);
StructuredBuffer<uint> readAliveCount : register(t17);         // Alive count per emitter (read side)
StructuredBuffer<ParticleFrameConsts> frameConsts : register(t18);
StructuredBuffer<ParticleConsts> consts : register(t19);
StructuredBuffer<float3> spawnPositions : register(t20);
StructuredBuffer<EmitterID> emitterIDs : register(t21);
StructuredBuffer<ParticleMeshConsts> meshConsts : register(t22);
StructuredBuffer<uint> readAliveIndices : register(t23);       // Read alive indices (simulation input)

StructuredBuffer<uint> batchEmitterList : register(t24);
StructuredBuffer<BatchDescriptor> batchDescriptors : register(t25);

// Curl Noise sampling (Tiling Mode)
float3 SampleCurlNoiseTiling(float3 worldPos, float frequency, float3 scrollSpeed, float time) {
    // ������ [0.0, 1.0]���� ����
    float3 scroll = frac(scrollSpeed * time);
    float3 uvw = (worldPos * frequency) + scroll;
    float4 curlData = curlNoiseTexture.SampleLevel(curlNoiseSampler, uvw, 0);
    return curlData.xyz * curlData.w; // ���� �� ���� = Curl ����
}

float2 SampleCurlNoise2D(float2 uv, float frequency, float2 scrollSpeed, float time) {
    float2 scroll = frac(scrollSpeed * time);
    float2 uvCoord = (uv * frequency) + scroll;
    float4 curlData = curlNoiseTexture2D.SampleLevel(curlNoiseSampler, uvCoord, 0);
    return curlData.rg;  // xy ���� �� ũ��
}

struct SortElement
{
    float key;
    uint value;
};

struct BatchSortParam {
    uint baseOffset;
    uint particleCount;
};

#endif // __PARTICLE_COMMON_HLSLI__