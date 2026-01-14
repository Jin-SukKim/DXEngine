#include "Common.hlsli"
#include "BlinnPhong.hlsli"

Texture2D g_albedoTex : register(t0);
Texture2D g_normalTex : register(t1);
Texture2D g_aoTex : register(t2);
Texture2D g_metallicRoughnessTex : register(t3);
Texture2D emissiveTex : register(t4);

float3 LinearToneMapping(float3 color) {
    float3 invGamma = float3(1, 1, 1) / 1.0; // Gamma Correction

    color = clamp(1.0 * color, 0., 1.);
    color = pow(color, invGamma);
    return color;
}

PSOutput main(PSInput input) {
    float3 toEye = normalize(eyeWorld - input.posWorld);
    
    float maxDist = 7.0f;
    float minDist = 3.0;
    
    float dist = length(eyeWorld - input.posWorld);
    float lod = 10.0 * saturate((dist - minDist) / (maxDist - minDist));
    
    // https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-for
    // https://forum.unity.com/threads/what-are-unroll-and-loop-when-to-use-them.1283096/
    float3 normalWorld = input.normalWorld;
    
    if (useNormalMap) {
        // Texture 좌표계의 Noraml Vector
        float3 normalTex = g_normalTex.SampleLevel(linearWrapSampler, input.texcoord, 0).rgb;
        normalTex = 2.0 * normalTex - 1.0; // 모든 방향에 대응하기 위해 [-1.0, 1.0]으로 변환
    
        // Texture 좌표계에 정의된 Normal Vector를 World 좌표계로 변환
        float3 N = normalWorld;
        float3 T = normalize(input.tangentWorld - dot(input.tangentWorld, N) * N); // Tangent
        float3 B = cross(N, T); // Bitangent
    
        // Texture 좌표계의 좌표축 방향을 World 좌표계로 변환한 3개의 벡털르 가지고 변환행렬 생성
        float3x3 TBN = float3x3(T, B, N);
        // Texture 좌표계의 Normal Vector를 World 좌표계로 변환행렬로 변환
        normalWorld = normalize(mul(normalTex, TBN));
    }
    
    float4 albedo = useAlbedoMap ?
        g_albedoTex.SampleLevel(linearWrapSampler, input.texcoord, lod) * float4(albedoFactor, 1.0)
        : float4(albedoFactor, 1.0);
    
    float ao = useAOMap ?
        g_aoTex.SampleLevel(linearWrapSampler, input.texcoord, lod).r
        : metallicFactor;
    
    float3 directLight = float3(0.0, 0.0, 0.0); // 간접광
    [unroll] // warning X3557: loop only executes for 1 iteration(s), forcing loop to unroll
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (lights[i].type == LIGHT_DIRECTIONAL)
            directLight += ComputeDirectionalLight(lights[i], normalWorld, toEye);
        else if (lights[i].type == LIGHT_POINT)
            directLight += ComputePointLight(lights[i], input.posWorld, normalWorld, toEye);
        else if (lights[i].type == LIGHT_SPOT)
            directLight += ComputeSpotLight(lights[i], input.posWorld, normalWorld, toEye);
        else
            continue;
    }
    
    // IBL (간접광)
    //float4 diffuseColor = irradianceIBLTex.Sample(linearWrapSampler, reflect(-toEye, normalWorld)) + float4(directLight, 1.0);
    float4 diffuseColor = irradianceIBLTex.Sample(linearWrapSampler, normalWorld) + float4(directLight, 1.0);
    diffuseColor *= float4(diffuse, 1.0);
    diffuseColor *= albedo * ao;
    
    float4 specularColor = specularIBLTex.Sample(linearWrapSampler, reflect(-toEye, normalWorld));
    specularColor *= pow(abs(specular.r + specular.g + specular.b) / 3.0, shininess);
    specularColor *= float4(specular, 1.0);
    
    // Schlick-Fresnel Effect (specular에 곱해줘서 물체의 안쪽은 물체의 색을 가장자리에 가까울수록 specular의 강도가 세지도록 계산)
    specularColor.xyz *= SchlickFresnel(fresnelR0, normalWorld, toEye);
    
    PSOutput output;
    //output.pixelColor = diffuseColor + specularColor;
    output.pixelColor = float4(LinearToneMapping(albedo.xyz), 1.0);
    output.indexColor = HashIdToColor(hashID);
    
    return output;
}