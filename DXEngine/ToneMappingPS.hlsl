#include "ToneMapping.hlsli"

Texture2D g_curFrame : register(t0);
SamplerState g_sampler : register(s0);

// 어떤 ToneMapping을 사용할지
cbuffer ToneMappingConsts : register(b1) {
    int useLinear; 
    int useFilmic; 
    int useUncharted2;
    int useLumaBasedReinhard;
};

struct SamplingPSInput {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD;
};

float4 main(SamplingPSInput input) : SV_TARGET
{
    // Tone Mapping을 사용 안하면 그냥 Copy 필터
    float3 color = g_curFrame.Sample(g_sampler, input.texcoord).rgb;
    color = useLinear ? LinearToneMapping(color) : color;
    color = useFilmic ? FilmicToneMapping(color) : color;
    color = useUncharted2 ? Uncharted2ToneMapping(color) : color;
    color = useLumaBasedReinhard ? lumaBasedReinhardToneMapping(color) : color;
    return float4(color, 1.0);
}