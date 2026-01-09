#include "Common.hlsli"

struct GSInput
{
    float4 pos : SV_POSITION;
    float3 color : COLOR;
    float life : PSIZE0;
    float size : PSIZE1;
};

struct ParticlePSInput
{
    float4 pos : SV_POSITION;
    float4 posWorld : POSITION0;
    float4 center : POSITION1;
    float2 texcoord : TEXCOORD;
    float3 color : COLOR;
    uint primID : SV_PrimitiveID;
};

[maxvertexcount(4)]
void main(
	point GSInput input[1], uint primID : SV_PrimitiveID,
	inout TriangleStream<ParticlePSInput> outputStream
)
{
    float hw = input[0].size * 0.5f;
    // 월드 좌표계의 up 축
    float4 up = float4(0.0, 1.0, 0.0, 0.0);
    float4 front = float4(eyeWorld, 1.0) - input[0].pos; // Point - Point = Vector
    front.w = 0.0; // 벡터
    float4 right = normalize(float4(cross(up.xyz, normalize(front.xyz)), 0.0));

    ParticlePSInput output;
    output.pos.w = 1;
    output.color = input[0].color;
    output.primID = primID;

    output.center = input[0].pos; // 빌보드의 중심

    // 왼쪽 아래 Point
    output.posWorld = input[0].pos - hw * right - hw * up;
    output.pos = output.posWorld;
    output.pos = mul(output.pos, view);
    output.pos = mul(output.pos, proj);
    output.texcoord = float2(1.0, 1.0);
    output.primID = primID;

    outputStream.Append(output);

    // 왼쪽 위 Point
    output.posWorld = input[0].pos - hw * right + hw * up;
    output.pos = output.posWorld;
    output.pos = mul(output.pos, view);
    output.pos = mul(output.pos, proj);
    output.texcoord = float2(1.0, 0.0);
    output.primID = primID;

    outputStream.Append(output);

    // 오른쪽 아래 Point
    output.posWorld = input[0].pos + hw * right - hw * up;
    output.pos = output.posWorld;
    output.pos = mul(output.pos, view);
    output.pos = mul(output.pos, proj);
    output.texcoord = float2(0.0, 1.0);
    output.primID = primID;

    outputStream.Append(output);

    // 오른쪽 위 Point
    output.posWorld = input[0].pos + hw * right + hw * up;
    output.pos = output.posWorld;
    output.pos = mul(output.pos, view);
    output.pos = mul(output.pos, proj);
    output.texcoord = float2(0.0, 0.0);
    output.primID = primID;

    outputStream.Append(output);

    outputStream.RestartStrip(); // Strip을 다시 시작
}