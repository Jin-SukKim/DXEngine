#include "Common.hlsli"

struct GSInput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float life : PSIZE0;
    float lifeRatio : TEXCOORD0;
    float size : PSIZE1;
    float rotation : PSIZE2;
    nointerpolation uint emitterID : BLENDINDICES;
};

struct ParticlePSInput
{
    float4 pos : SV_POSITION;
    float4 posWorld : POSITION0;
    float4 center : POSITION1;
    float2 texcoord : TEXCOORD;
    float4 color : COLOR;
    float lifeRatio : TEXCOORD1;
    uint primID : SV_PrimitiveID;
    nointerpolation uint emitterID : BLENDINDICES;
};

float2x2 GetRotationMatrix(float angle) {
    float c = cos(angle);
    float s = sin(angle);

    return float2x2(
        c, -s,
        s, c
    );
}

[maxvertexcount(4)]
void main(
	point GSInput input[1], uint primID : SV_PrimitiveID,
	inout TriangleStream<ParticlePSInput> outputStream
)
{
    ParticlePSInput output;
    output.primID = primID;
    output.color = input[0].color;
    output.lifeRatio = input[0].lifeRatio;
    output.emitterID = input[0].emitterID;

    output.posWorld = input[0].pos;
    output.center = input[0].pos;
    float4 viewPos = mul(float4(input[0].pos.xyz, 1.f), view);
    float hw = input[0].size * 0.5f;

    // View space������ offset ����
    float2 offsets[4] = {
        float2(-1.f, -1.f),
        float2(-1.f, 1.f),
        float2(1.f, -1.f),
        float2(1.f, 1.f)
    };

    float2 uvs[4] = {
        float2(0.f, 1.f),
        float2(0.f, 0.f),
        float2(1.f, 1.f),
        float2(1.f, 0.f)
    };

    float2x2 rotMatrix = GetRotationMatrix(input[0].rotation);

    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        // View space���� billboard ��ġ ����
        float4 newPos = viewPos;
        float2 offset = mul(rotMatrix, offsets[i]);
        newPos.xy += offset * hw;

        // TODO: ���� 2D ȸ���� �ְ� �ʹٸ� ���⼭ offsets[i]�� ȸ�� ��ķ� ������

        output.pos = mul(newPos, proj);
        output.texcoord = uvs[i];

        outputStream.Append(output);
    }

    outputStream.RestartStrip(); // Strip�� �ٽ� ����
}