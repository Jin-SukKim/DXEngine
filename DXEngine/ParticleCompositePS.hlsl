Texture2D g_texture0 : register(t0);
SamplerState g_sampler : register(s0);

struct SamplingPSInput {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD;
};

float4 main(SamplingPSInput input) : SV_TARGET {
    float4 color = g_texture0.Sample(g_sampler, input.texcoord);
    return float4(color.rgb, 1.0);
}
