#include "Common.hlsli"
#include "Particle.hlsli"

struct SortElement
{
    float key;
    uint value;
};

StructuredBuffer<Particle> particles : register(t0);
StructuredBuffer<SortElement> sortedElements : register(t1);

struct GSInput
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
    float life : PSIZE0;
    float size : PSIZE1;
    float rotation : PSIZE2;
};

// 3D Euler -> Rotation Matrix
float3x3 GetRotationMatrix(float3 rot) {
    float cX = cos(rot.x), sX = sin(rot.x);
    float cY = cos(rot.y), sY = sin(rot.y);
    float cZ = cos(rot.z), sZ = sin(rot.z);

    // Z * Y * X ¼ø¼­ (Roll -> Yaw -> Pitch)
    float3x3 mX = { 1, 0, 0,  0, cX, -sX,  0, sX, cX };
    float3x3 mY = { cY, 0, sY,  0, 1, 0,  -sY, 0, cY };
    float3x3 mZ = { cZ, -sZ, 0,  sZ, cZ, 0,  0, 0, 1 };

    return mul(mZ, mul(mY, mX));
}

GSInput main(uint vertexID : SV_VertexID)
{
    uint particleIdx = sortedElements[vertexID].value;
    Particle p = particles[particleIdx];

    GSInput output;

    output.position = float4(p.position.xyz, 1.0);

    /*float3x3 rotMatrix = GetRotationMatrix(p.rotation);
    output.position = float4(mul(rotMatrix, p.position.xyz), 1.0);*/

    output.rotation = p.rotation.x;
    output.color = p.color;
    output.life = p.life;
    output.size = p.size;

    return output;
}