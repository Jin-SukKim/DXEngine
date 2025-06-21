#include "Common.hlsli"

Texture2D curFrame : register(t0);

struct SamplingPSInput {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD;
};

float4 main(SamplingPSInput input) : SV_TARGET {
    return curFrame.Sample(linearWrapSampler, input.texcoord);
}