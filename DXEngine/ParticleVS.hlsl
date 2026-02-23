#include "Common.hlsli"
#include "ParticleCommon.hlsli"

StructuredBuffer<SortElement> sortedElements : register(t1);

struct GSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float life : PSIZE0;
    float lifeRatio : TEXCOORD0;
    float size : PSIZE1;
    float rotation : PSIZE2;
    float3 normal : NORMAL0;
    nointerpolation uint normalBillboard : BLENDINDICES0;
};

// 3D Euler -> Rotation Matrix
float3x3 GetRotationMatrix(float3 rot) {
    float cX = cos(rot.x), sX = sin(rot.x);
    float cY = cos(rot.y), sY = sin(rot.y);
    float cZ = cos(rot.z), sZ = sin(rot.z);

    // Z * Y * X ���� (Roll -> Yaw -> Pitch)
    float3x3 mX = { 1, 0, 0,  0, cX, -sX,  0, sX, cX };
    float3x3 mY = { cY, 0, sY,  0, 1, 0,  -sY, 0, cY };
    float3x3 mZ = { cZ, -sZ, 0,  sZ, cZ, 0,  0, 0, 1 };

    return mul(mZ, mul(mY, mX));
}

GSInput main(uint vertexID : SV_VertexID)
{
    RenderConsts render = consts[emitterID].render;
    uint particleIdx = render.useSorting ? sortedElements[vertexID].value : vertexID;

    Particle p = readParticles[readParticleOffset + particleIdx];

    GSInput output;

    SpawnConsts spawn = consts[emitterID].spawn;
    // ����/���� ��忡 ���� ������ ��ġ ����
    if (spawn.simulationSpace == 1)
    {
        // �̹� World ��ǥ�̹Ƿ� View-Projection ��ȯ�� �����ϸ� ��
        // ������ VS������ World ��� ���� �����ϰ�, float4(p.position, 1.0)�� �ѱ�
        // (Pixel Shader�� Geometry Shader �ܰ迡�� View/Proj�� ����� ����)
        // �� �ڵ�� VSInput -> GSInput �ܰ��̹Ƿ� World ��ȯ ���θ� ����
        output.position = float4(p.position.xyz, 1.0);
    }
    else
    {
        // ����: Local ��ǥ�̹Ƿ� World ��� ���� �ʿ�
        output.position = mul(float4(p.position.xyz, 1.0), meshConsts[p.systemID].pWorld);
    }

    /*float3x3 rotMatrix = GetRotationMatrix(p.rotation);
    output.position = float4(mul(rotMatrix, p.position.xyz), 1.0);*/

    output.rotation = p.rotation.x;
    output.color = p.color;
    output.life = p.life;
    output.lifeRatio = 1.0 - saturate(p.life / p.lifeMax);
    output.size = p.size;

    if (spawn.simulationSpace == 1)
        output.normal = p.normal; // Already world space
    else
        output.normal = normalize(mul(p.normal, (float3x3) meshConsts[p.systemID].pWorldIT));

    output.normalBillboard = consts[emitterID].render.normalBillboard;

    return output;
}