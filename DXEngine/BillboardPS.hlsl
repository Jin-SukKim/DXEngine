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
    float maxDist = 7.0f;
    float minDist = 3.0;
    
    float dist = length(eyeWorld - input.posWorld.xyz);
    float lod = 10.0 * saturate((dist - minDist) / (maxDist - minDist));
    
    float4 color = { 1.0, 0.0, 0.0, 1.0 };
    // TextureArray이기 때문에 3차원 Texture 좌표를 사용
    // 2차원 Texture 좌표에 어떤 Texture를 사용할지 index값을 3차원 값으로 넣어줌
    float3 uvw = float3(input.texcoord, float(input.primID % arraySize));
    color = g_texArray.SampleLevel(linearWrapSampler, uvw, lod);
    
    // clip(x)에서 x가 0보다 작으면 이 픽셀의 색은 버림
    clip((color.a < 0.9f) || (color.r + color.g + color.b) > 2.4 ? -1 : 1);
    
    return color;
}