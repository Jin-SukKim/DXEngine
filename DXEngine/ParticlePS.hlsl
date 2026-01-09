#include "Common.hlsli"

struct ParticlePSInput
{
    float4 pos : SV_POSITION;
    float4 posWorld : POSITION0;
    float4 center : POSITION1;
    float2 texCoord : TEXCOORD;
    float3 color : COLOR;
    uint primID : SV_PrimitiveID;
};

float4 main(ParticlePSInput input) : SV_TARGET
{
    float dist = length(float2(0.5, 0.5) - input.texCoord) * 2;
    float scale = saturate(1.0 - dist);
    
    return float4(input.color.rgb * scale, 1.0);
}