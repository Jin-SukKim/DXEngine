#include "Common.hlsli"
#include "BlinnPhong.hlsli"

Texture2D g_texture0 : register(t0);
SamplerState g_sampler : register(s0);

float4 main(PSInput input) : SV_TARGET {
    float3 toEye = normalize(eyeWorld - input.posWorld);
    float3 color = float3(0.0, 0.0, 0.0);
    
    // https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-for
    // https://forum.unity.com/threads/what-are-unroll-and-loop-when-to-use-them.1283096/
    
    [unroll] // warning X3557: loop only executes for 1 iteration(s), forcing loop to unroll
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (lights[i].type == LIGHT_DIRECTIONAL)
            color += ComputeDirectionalLight(lights[i], input.normalWorld, toEye);
        else if (lights[i].type == LIGHT_POINT)
            color += ComputePointLight(lights[i], input.posWorld, input.normalWorld, toEye);
        else if (lights[i].type == LIGHT_SPOT)
            color += ComputeSpotLight(lights[i], input.posWorld, input.normalWorld, toEye);
        else
            continue;
    }
    
    return float4(color, 1.0) * g_texture0.Sample(g_sampler, input.texcoord);
}