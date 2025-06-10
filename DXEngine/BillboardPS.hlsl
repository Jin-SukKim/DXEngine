#include "Common.hlsli"

Texture2D g_texture0 : register(t0);

struct BillboardPSInput {
    float4 pos : SV_POSITION;
    float4 posWorld : POSITION0;
    float4 center : POSITION1;
    float2 texcoord : TEXCOORD;
    uint primID : SV_PrimitiveID;
};

float4 main(BillboardPSInput input) : SV_TARGET {
    float4 color = { 0.0, 0.0, 0.0, 1.0 };
    
    color = g_texture0.Sample(linearWrapSampler, input.texcoord);
    
    // clip(x)에서 x가 0보다 작으면 이 픽셀의 색은 버림
    clip(color.a - 0.9);
    
    return color;
}