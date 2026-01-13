#include "Common.hlsli"

struct ParticlePSInput
{
    float4 pos : SV_POSITION;
    float4 posWorld : POSITION0;
    float4 center : POSITION1;
    float2 texCoord : TEXCOORD;
    float4 color : COLOR;
    uint primID : SV_PrimitiveID;
};

float4 main(ParticlePSInput input) : SV_TARGET
{
    float dist = length(float2(0.5, 0.5) - input.texCoord) * 2;
    float scale = saturate(1.0 - dist);
    
    if (scale <= 0.0)
        discard; // 0이면 아예 그리지 않음 (Depth Buffer 오염 방지 등)

    // [수정 전] Alpha가 1.0이라서 배경을 다 지워버림
    //return float4(input.color.rgb * scale, 1.0);

    // [수정 후] RGB에도 scale을 곱하고(Pre-multiplied), Alpha에도 scale을 적용
    return float4(input.color.rgb * scale, input.color.a);
}