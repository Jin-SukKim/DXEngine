Texture2D g_texture0 : register(t0);
SamplerState g_sampler : register(s0);

cbuffer SamplingPSConstantData : register(b0) {
    // Texture의 Pixel 간격 (설정한 해상도에 따라 dx, dy값은 다름)
    float dx; 
    float dy;
    float threadhold;
    float strength;
    float4 options; // 여기서 Option은 사용하지 않음
}

struct SamplingPSInput {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD;
};

float4 main(SamplingPSInput input) : SV_TARGET
{
    float x = input.texcoord.x;
    float y = input.texcoord.y;
    
    float3 a = g_texture0.Sample(g_sampler, float2(x - 2 * dx, y + 2 * dy)).rgb;
    float3 b = g_texture0.Sample(g_sampler, float2(x, y + 2 * dy)).rgb;
    float3 c = g_texture0.Sample(g_sampler, float2(x + 2 * dx, y + 2 * dy)).rgb;

    float3 d = g_texture0.Sample(g_sampler, float2(x - 2 * dx, y)).rgb;
    float3 e = g_texture0.Sample(g_sampler, float2(x, y)).rgb;
    float3 f = g_texture0.Sample(g_sampler, float2(x + 2 * dx, y)).rgb;

    float3 g = g_texture0.Sample(g_sampler, float2(x - 2 * dx, y - 2 * dy)).rgb;
    float3 h = g_texture0.Sample(g_sampler, float2(x, y - 2 * dy)).rgb;
    float3 i = g_texture0.Sample(g_sampler, float2(x + 2 * dx, y - 2 * dy)).rgb;

    float3 j = g_texture0.Sample(g_sampler, float2(x - dx, y + dy)).rgb;
    float3 k = g_texture0.Sample(g_sampler, float2(x + dx, y + dy)).rgb;
    float3 l = g_texture0.Sample(g_sampler, float2(x - dx, y - dy)).rgb;
    float3 m = g_texture0.Sample(g_sampler, float2(x + dx, y - dy)).rgb;

    float3 color = e * 0.125;
    color += (a + c + g + i) * 0.03125;
    color += (b + d + f + h) * 0.0625;
    color += (j + k + l + m) * 0.125;
  
    return float4(color, 1.0);
}