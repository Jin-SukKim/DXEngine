#include "ToneMapping.hlsli"

Texture2D g_texture0 : register(t0);
Texture2D g_texture1 : register(t1);
Texture2D g_prevFrame : register(t2); // 이전 Frame 렌더링 결과
SamplerState g_sampler : register(s0);

struct SamplingPSInput {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD;
};

float4 main(SamplingPSInput input) : SV_TARGET {
    float3 color0 = g_texture0.Sample(g_sampler, input.texcoord).rgb;
    float3 color1 = g_texture1.Sample(g_sampler, input.texcoord).rgb;
    
    float3 combined = (1.0 - strength) * color0 + strength * color1;
    
    // Tone Mapping 적용
    combined = LinearToneMapping(combined);
    
    // 모션 블러
    combined = lerp(combined, g_prevFrame.Sample(g_sampler, input.texcoord).rgb, blur);
    
	return float4(combined, 1.0f);
}