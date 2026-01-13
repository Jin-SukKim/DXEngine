#include "Common.hlsli"

struct GSInput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float life : PSIZE0;
    float size : PSIZE1;
    float rotation : PSIZE2;
};

struct ParticlePSInput
{
    float4 pos : SV_POSITION;
    float4 posWorld : POSITION0;
    float4 center : POSITION1;
    float2 texcoord : TEXCOORD;
    float4 color : COLOR;
    uint primID : SV_PrimitiveID;
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

    output.posWorld = input[0].pos;
    output.center = input[0].pos;
    float4 viewPos = mul(float4(input[0].pos.xyz, 1.f), view);
    float hw = input[0].size * 0.5f;

    // View space에서의 offset 정의
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
        // View space에서 billboard 위치 결정
        float4 newPos = viewPos;
        float2 offset = mul(rotMatrix, offsets[i]);
        newPos.xy += offset * hw;

        // TODO: 만약 2D 회전을 넣고 싶다면 여기서 offsets[i]를 회전 행렬로 돌리기

        output.pos = mul(newPos, proj);
        output.texcoord = uvs[i];

        outputStream.Append(output);
    }

    outputStream.RestartStrip(); // Strip을 다시 시작
}