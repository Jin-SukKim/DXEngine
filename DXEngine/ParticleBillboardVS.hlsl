#define PARTICLE_RENDER_STAGE
#include "Common.hlsli"
#include "ParticleCommon.hlsli"

StructuredBuffer<SortElement> sortedElements : register(t1);

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
};

// 2D Rotation Matrix
float2x2 GetRotationMatrix2D(float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    return float2x2(c, -s, s, c);
}

// Resolve global instanceID to (emitterSlotID, local particle index)
void ResolveBatchInstance(uint instanceID, out uint emitterSlotID, out uint localIdx)
{
    uint accumulatedCount = 0;

    // Linear scan through emitters in this batch
    for (uint i = 0; i < batchEmitterCount; i++) {
        uint eid = batchEmitterList[batchEmitterListOffset + i];

        // Get actual instance count for this emitter
        uint count = uint(float(readCount[eid]) * frameConsts[eid].spawnRatio);

        if (instanceID < accumulatedCount + count) {
            emitterSlotID = eid;
            localIdx = instanceID - accumulatedCount;
            return;
        }
        accumulatedCount += count;
    }

    // Fallback (shouldn't happen)
    emitterSlotID = 0;
    localIdx = 0;
}

ParticlePSInput main(VSParticleInput input)
{
    // Resolve instance to emitter
    uint emitterSlotID;
    uint localIdx;
    ResolveBatchInstance(input.instanceID, emitterSlotID, localIdx);

    // Use resolved emitter data instead of global emitterID
    RenderConsts render = consts[emitterSlotID].render;
    uint particleIdx = render.useSorting ? sortedElements[localIdx].value : localIdx;

    // Get emitter data from emitterIDs buffer
    EmitterID emitterData = emitterIDs[emitterSlotID];
    Particle p = readParticles[emitterData.readParticleOffset + particleIdx];

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
    output.uv = input.texcoord;

    // View Space로 변환
    float4 viewPos = mul(particleCenter, view);

    // 쿼드 로컬 오프셋 (-1~1 범위)
    float2 quadOffset = input.position.xy; // MakeSquare()는 -1~1 범위

    // 회전 적용
    float2x2 rotMatrix = GetRotationMatrix2D(p.rotation.x);
    float2 rotatedOffset = mul(rotMatrix, quadOffset);

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

    viewPos.xy += rotatedOffset * halfSize;

    // Projection
    output.pos = mul(viewPos, proj);
    output.posWorld = particleCenter;

    return output;
}
