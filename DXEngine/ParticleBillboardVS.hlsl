#define PARTICLE_RENDER_STAGE
#include "Common.hlsli"
#include "ParticleCommon.hlsli"

StructuredBuffer<SortElement> sortedElements : register(t1);
StructuredBuffer<uint> aliveIndices : register(t26);
StructuredBuffer<uint> emitterWriteOffsets : register(t27);

// Vertex Input (Quad Mesh)
struct VSParticleInput
{
    float3 position : POSITION;
    float3 normalModel : NORMAL;
    float2 texcoord : TEXCOORD;
    float3 tangentModel : TANGENT;
    uint instanceID : SV_InstanceID;
};

// Pixel Shader Input (ParticlePS.hlsl과 일치)
struct ParticlePSInput
{
    float4 pos : SV_POSITION;
    float4 posWorld : POSITION0;
    float4 center : POSITION1;
    float2 uv : TEXCOORD0;
    float4 color : COLOR;
    float lifeRatio : TEXCOORD1;
    uint emitterSlotID : TEXCOORD2;  // Pass emitter ID to PS for sprite animation
    float lifeMax : TEXCOORD3;
};

// 2D Rotation Matrix
float2x2 GetRotationMatrix2D(float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    return float2x2(c, -s, s, c);
}

ParticlePSInput main(VSParticleInput input)
{
    // Batch에서 EmitterList의 시작 offset을 가져오기
    uint batchStartOffset = emitterWriteOffsets[batchEmitterListOffset];
    // EmitterLIst의 시작 offset에 instanceID를 더해 particle의 index 가져오기
    uint globalIdx = aliveIndices[batchStartOffset + input.instanceID];
    Particle p = readParticles[globalIdx];
    uint emitterSlotID = p.ownerID;

    ParticlePSInput output;

    // Pass emitterSlotID to Pixel Shader for correct sprite animation
    output.emitterSlotID = emitterSlotID;

    // 파티클 중심 위치 (World/Local Space)
    SpawnConsts spawn = consts[emitterSlotID].spawn;
    float4 particleCenter;
    if (spawn.simulationSpace == 1)
    {
        // World Space
        particleCenter = float4(p.position.xyz, 1.0);
    }
    else
    {
        // Local Space
        particleCenter = mul(float4(p.position.xyz, 1.0), meshConsts[p.systemID].pWorld);
    }

    output.center = particleCenter;
    output.color = p.color;
    output.lifeRatio = 1.0 - saturate(p.life / p.lifeMax);
    output.lifeMax = p.lifeMax;
    output.uv = input.texcoord;

    // View Space로 변환
    float4 viewPos = mul(particleCenter, view);

    // 쿼드 로컬 오프셋 (-1~1 범위)
    float2 quadOffset = input.position.xy; // MakeSquare()는 -1~1 범위

    // 크기 적용 (파티클 크기의 절반)
    float halfSize = p.size * 0.5;

    // Per-emitter size scaling based on distance
    VisualConsts visual = consts[emitterSlotID].visual;
    if (visual.enableSizeScaling) {
        float camDist = distance(particleCenter.xyz, eyeWorld);
        float t = saturate((camDist - visual.sizeDistanceMin) /
                          (visual.sizeDistanceMax - visual.sizeDistanceMin));
        float sizeFactor = lerp(visual.sizeDistanceScale, 1.0f, t);
        halfSize *= sizeFactor;
    }

    RenderConsts render = consts[emitterSlotID].render;

    if (render.normalBillboard)
    {
        // Normal-based billboard: orient quad along mesh surface normal
        float3 worldNormal;
        if (spawn.simulationSpace == 1)
            worldNormal = p.normal; // Already world space (transformed in SpawnCS)
        else
            worldNormal = normalize(mul(p.normal, (float3x3) meshConsts[p.systemID].pWorldIT));

        float3 billNormal = normalize(worldNormal);
        float3 worldUp = float3(0, 1, 0);
        float3 right = cross(worldUp, billNormal);
        if (length(right) < 0.001f) right = float3(1, 0, 0); // degenerate fallback
        right = normalize(right);
        float3 up = normalize(cross(billNormal, right));

        float2x2 rotMatrix = GetRotationMatrix2D(p.rotation.x);
        float2 rotatedOffset = mul(rotMatrix, quadOffset);

        float3 worldCorner = particleCenter.xyz + rotatedOffset.x * halfSize * right + rotatedOffset.y * halfSize * up;
        float4 worldCornerViewPos = mul(float4(worldCorner, 1.0), view);
        output.pos = mul(worldCornerViewPos, proj);
        output.posWorld = float4(worldCorner, 1.0);
    }
    else
    {
        // Camera-facing billboard (view space)
        // Velocity Stretch Billboard
        float stretchFactor = render.velocityStretchFactor;
        float3 worldVel = p.velocity;
        if (spawn.simulationSpace == 0)
        {
            // Local Space → World Space
            worldVel = mul(float4(worldVel, 0.0), meshConsts[p.systemID].pWorld).xyz;
        }
        float3 viewVel = mul(float4(worldVel, 0.0), view).xyz;
        float speed2D = length(viewVel.xy);

        if (stretchFactor > 0.0 && speed2D > 0.001)
        {
            float2 velDir = viewVel.xy / speed2D;
            float2 velPerp = float2(-velDir.y, velDir.x);

            float stretchAmount = 1.0 + speed2D * stretchFactor;
            viewPos.xy += (quadOffset.x * velDir * stretchAmount
                         + quadOffset.y * velPerp) * halfSize;
        }
        else
        {
            float2x2 rotMatrix = GetRotationMatrix2D(p.rotation.x);
            float2 rotatedOffset = mul(rotMatrix, quadOffset);
            viewPos.xy += rotatedOffset * halfSize;
        }

        output.pos = mul(viewPos, proj);
        output.posWorld = particleCenter;
    }

    return output;
}
