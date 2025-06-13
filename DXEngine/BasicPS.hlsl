#include "Common.hlsli"
#include "BlinnPhong.hlsli"

Texture2D g_texture0 : register(t0);

PSOutput main(PSInput input) {
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
    
    float maxDist = 7.0f;
    float minDist = 3.0;
    
    float dist = length(eyeWorld - input.posWorld);
    float lod = 10.0 * saturate((dist - minDist) / (maxDist - minDist));
    
    PSOutput output;
    output.pixelColor = float4(color, 1.0) * g_texture0.SampleLevel(linearWrapSampler, input.texcoord, lod);
    output.indexColor = HashIdToColor(hashID);
    
    return output;
}