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
    RenderConsts render = consts[emitterID].render;

    // 파티클 인덱스 (정렬 여부에 따라)
    uint particleIdx = render.useSorting ? sortedElements[input.instanceID].value : input.instanceID;

    // 파티클 데이터 로드
    Particle p = readParticles[readParticleOffset + particleIdx];

    ParticlePSInput output;

    // 파티클 중심 위치 (World/Local Space)
    SpawnConsts spawn = consts[emitterID].spawn;
    float4 particleCenter;
    if (spawn.simulationSpace == 1)
    {
        // World Space
        particleCenter = float4(p.position.xyz, 1.0);
    }
    else
    {
        // Local Space
        particleCenter = mul(float4(p.position.xyz, 1.0), pWorld);
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
    viewPos.xy += rotatedOffset * halfSize;

    // Projection
    output.pos = mul(viewPos, proj);
    output.posWorld = particleCenter;

    return output;
}
