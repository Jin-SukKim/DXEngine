#include "Common.hlsli"
#include "ParticleCommon.hlsli"

StructuredBuffer<Particle> particles : register(t0);
StructuredBuffer<SortElement> sortedElements : register(t1);

struct GSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float life : PSIZE0;
    float lifeRatio : TEXCOORD0;
    float size : PSIZE1;
    float rotation : PSIZE2;
};

// 3D Euler -> Rotation Matrix
float3x3 GetRotationMatrix(float3 rot) {
    float cX = cos(rot.x), sX = sin(rot.x);
    float cY = cos(rot.y), sY = sin(rot.y);
    float cZ = cos(rot.z), sZ = sin(rot.z);

    // Z * Y * X 순서 (Roll -> Yaw -> Pitch)
    float3x3 mX = { 1, 0, 0,  0, cX, -sX,  0, sX, cX };
    float3x3 mY = { cY, 0, sY,  0, 1, 0,  -sY, 0, cY };
    float3x3 mZ = { cZ, -sZ, 0,  sZ, cZ, 0,  0, 0, 1 };

    return mul(mZ, mul(mY, mX));
}

GSInput main(uint vertexID : SV_VertexID)
{
    uint particleIdx = render.useSorting ? sortedElements[vertexID].value : vertexID;
    Particle p = particles[particleIdx];

    GSInput output;

    // 로컬/월드 모드에 따라 렌더링 위치 결정
    if (spawn.simulationSpace == 1)
    {
        // 이미 World 좌표이므로 View-Projection 변환만 적용하면 됨
        // 하지만 VS에서는 World 행렬 곱을 생략하고, float4(p.position, 1.0)을 넘김
        // (Pixel Shader나 Geometry Shader 단계에서 View/Proj가 적용될 것임)
        // 이 코드는 VSInput -> GSInput 단계이므로 World 변환 여부만 제어
        output.position = float4(p.position.xyz, 1.0);
    }
    else
    {
        // 기존: Local 좌표이므로 World 행렬 곱셈 필요
        output.position = mul(float4(p.position.xyz, 1.0), pWorld);
    }

    /*float3x3 rotMatrix = GetRotationMatrix(p.rotation);
    output.position = float4(mul(rotMatrix, p.position.xyz), 1.0);*/

    output.rotation = p.rotation.x;
    output.color = p.color;
    output.life = p.life;
    output.lifeRatio = 1.0 - saturate(p.life / p.lifeMax);
    output.size = p.size;

    return output;
}