#include "Common.hlsli"

struct GSInput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float life : PSIZE0;
    float lifeRatio : TEXCOORD0;
    float size : PSIZE1;
    float rotation : PSIZE2;
    float3 normal : NORMAL0;
    nointerpolation uint normalBillboard : BLENDINDICES0;
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
    output.center = input[0].pos;

    float hw = input[0].size * 0.5f;

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

    if (input[0].normalBillboard)
    {
        // Normal-based orientation (world space quad)
        float3 billNormal = normalize(input[0].normal);
        float3 worldUp = float3(0, 1, 0);
        float3 right = cross(worldUp, billNormal);
        if (length(right) < 0.001f) right = float3(1, 0, 0); // degenerate fallback
        right = normalize(right);
        float3 up = normalize(cross(billNormal, right));

        float2x2 rotMat = GetRotationMatrix(input[0].rotation);
        [unroll]
        for (int i = 0; i < 4; ++i)
        {
            float2 rotOffset = mul(rotMat, offsets[i]);
            float3 worldCorner = input[0].pos.xyz + rotOffset.x * hw * right + rotOffset.y * hw * up;
            float4 viewPos = mul(float4(worldCorner, 1.0), view);
            output.pos = mul(viewPos, proj);
            output.posWorld = float4(worldCorner, 1.0);
            output.texcoord = uvs[i];
            outputStream.Append(output);
        }
    }
    else
    {
        // Camera-facing billboard (view space)
        output.posWorld = input[0].pos;
        float4 viewPos = mul(float4(input[0].pos.xyz, 1.f), view);

        float2x2 rotMatrix = GetRotationMatrix(input[0].rotation);
        [unroll]
        for (int i = 0; i < 4; ++i)
        {
            float4 newPos = viewPos;
            float2 offset = mul(rotMatrix, offsets[i]);
            newPos.xy += offset * hw;

            output.pos = mul(newPos, proj);
            output.texcoord = uvs[i];

            outputStream.Append(output);
        }
    }

    outputStream.RestartStrip();
}
