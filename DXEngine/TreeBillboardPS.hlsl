#include "Common.hlsli"

// Texture Array
Texture2DArray g_texArray : register(t0);

cbuffer BillboardConsts : register(b3) {
    float widthWorld; // world width
    float3 dummy4;
    uint arraySize;
    float3 dummy5;
};

struct BillboardPSInput {
    float4 pos : SV_POSITION;
    float4 posWorld : POSITION0;
    float4 center : POSITION1;
    float2 texcoord : TEXCOORD;
    uint primID : SV_PrimitiveID;
};


float4 main(BillboardPSInput input) : SV_TARGET {
    // TextureArray이기 때문에 3차원 Texture 좌표를 사용
    // 2차원 Texture 좌표에 어떤 Texture를 사용할지 index값을 3차원 값으로 넣어줌
    float3 uvw = float3(input.texcoord, float(input.primID % arraySize));
    //color = g_texture0.Sample(linearWrapSampler, input.texcoord);
    float4 color = g_texArray.Sample(linearWrapSampler, uvw);
    
    // clip(x)에서 x가 0보다 작으면 이 픽셀의 색은 버림
    // 픽셀의 값이 흰색에 가까운 배경 색이면 clip
    clip((color.a < 0.9f) || (color.r + color.g + color.b) > 2.4 ? -1 : 1);
    
    return color;
}