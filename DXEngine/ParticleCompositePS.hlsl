#include "Common.hlsli"

Texture2D lowResTexture : register(t0);

struct SamplingPSInput {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD;
};

float4 main(SamplingPSInput input) : SV_TARGET{
    float4 color = lowResTexture.Sample(linearClampSampler, input.texcoord);
    return float4(color.rgb, 1.0);
}