#include "Common.hlsli"
#include "Shadow.hlsli"
// https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
Texture2D albedoTex : register(t0);
Texture2D normalTex : register(t1);
Texture2D aoTex : register(t2);
Texture2D metallicTex : register(t3);
Texture2D roughnessTex : register(t4);
Texture2D emissiveTex : register(t5);

// ü   F0 ϴµ metallic   Fdielectric albedo lerp  
static const float3 Fdielectric = 0.04; // ݼ  F0 ּҰ 0 ƴ 0.04

float3 GetNormal(PSInput input) {
    float3 normalWorld = input.normalWorld;
    
    if (useNormalMap) {
        // Texture ǥ Noraml Vector
        float3 normal = normalTex.SampleLevel(linearWrapSampler, input.texcoord, 0).rgb;
        normal = 2.0 * normal - 1.0; //  ⿡ ϱ  [-1.0, 1.0] ȯ
    
        // OpenGL  Normal Map 쿡 y  ֱ
        normal.y = invertNormalMapY ? -normal.y : normal.y;
        
        // Texture ǥ迡 ǵ Normal Vector World ǥ ȯ
        float3 N = normalWorld;
        float3 T = normalize(input.tangentWorld - dot(input.tangentWorld, N) * N); // Tangent
        float3 B = cross(N, T); // Bitangent
    
        // Texture ǥ ǥ  World ǥ ȯ 3 и  ȯ 
        float3x3 TBN = float3x3(T, B, N);
        // Texture ǥ Normal Vector World ǥ ȯķ ȯ
        normalWorld = normalize(mul(normal, TBN));
    }
    
    return normalWorld;
}

float3 SchlickFresnel(float3 F0, float NdotH) {
    return F0 + (1.0 - F0) * pow(2.0, (-5.55473 * NdotH - 6.98316) * NdotH);
    
    //  GPU ӵ   5 ൵   
    //return F0 + (1.0 - F0) * po   w(1.0 - cosTheta, 5.0);
}

float3 DiffuseIBL(float3 albedo, float3 normalWorld, float3 pixelToEye, float metallic) {
    // metallic ̿ Diffuse   (diffuse metallic  þ Specular پ   Ģ)
    float3 F0 = lerp(Fdielectric, albedo, metallic); // ü   F0 
    float3 F = SchlickFresnel(F0, max(0.0, dot(normalWorld, pixelToEye)));
    // metallic 1.0  kd  0  metal̸ diffuse پ鵵 
    float3 kd = lerp(1.0 - F, 0.0, metallic);
    
    float3 irradiance = irradianceIBLTex.SampleLevel(linearWrapSampler, normalWorld, 0.0).rgb;
    
    return kd * albedo * irradiance;
}

float3 SpecularIBL(float3 albedo, float3 normalWorld, float3 pixelToEye, float metallic, float roughness) {
    // Environement BRDF (Clamp Sampler ) - r = cos(theta), g = roughness
    // (IBLBaker   LUT  roughness ״ ϸ ȵǰ 1 - roughness ־ )
    float2 specularBRDF = brdfTex.SampleLevel(linearClampSampler, float2(dot(normalWorld, pixelToEye), 1.0 - roughness), 0.0f).rg;

    // Roughness ö󰡸  ػ Mipmap  Non-Metal  ǥ 
    // (lod level   roughness mipmap ְ ̳  ְ  ֱ)
    float3 specularIrradiance = specularIBLTex.SampleLevel(linearWrapSampler, reflect(-pixelToEye, normalWorld), roughness * 5.0f).rgb;
    // ü   F0 
    float3 F0 = lerp(Fdielectric, albedo, metallic);
    
    return (F0 * specularBRDF.x + specularBRDF.y) * specularIrradiance;

}

//  (Ambient Light) - ֺ ȯκ ޴ 
float3 AmbientLightingByIBL(float3 albedo, float3 normalWorld, float3 pixelToEye, float ao, float metallic, float roughness) {
    // metallic ̿ diffuse  
    float3 diffuseIBL = DiffuseIBL(albedo, normalWorld, pixelToEye, metallic);
    //   Ģ  diffuse þ specular پ  ݴ Ȳ 
    float3 specularIBL = SpecularIBL(albedo, normalWorld, pixelToEye, metallic, roughness);
    
    // ao ü    ʴ κ
    return (diffuseIBL + specularIBL) * ao;
}

// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2.
float NdfGGX(float NdotH, float roughness, float alphaPrime) {
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;
    float alphaPrimeSq = alphaPrime * alphaPrime;
    float denom = (NdotH * NdotH) * (alphaPrimeSq - 1.0) + 1.0;

    return alphaSq / (3.141592 * denom * denom);
}

// Single term for separable Schlick-GGX below.
float SchlickG1(float NdotV, float k) {
    return NdotV / (NdotV * (1.0 - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method.
float SchlickGGX(float NdotI, float NdotO, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return SchlickG1(NdotI, k) * SchlickG1(NdotO, k);
}

// Light Type   - Shadow Map   
float3 LightRadiance(Light light, float3 representativePoint, float3 posWorld, float3 normalWorld, uint shadowIdx) {
    // Directional Light
    float3 lightVec = light.type & LIGHT_DIRECTIONAL ?
                        // Directional Light   ״  (ex: ¾)
                        -light.direction :
                        representativePoint - posWorld; // (Area Light  Ѵٸ : light.position - posWorld;)
    
    float lightDist = length(lightVec); // Dirctional Light 1.0 Ǿ
    lightVec /= lightDist; // Normalize
    
    // Spot Light
    float spotFactor = light.type & LIGHT_SPOT ?
                        //  ϴ   ִ 
                        pow(max(-dot(lightVec, light.direction), 0.0), light.spotPower) :
                        1.0; // Directional Light̳ Point Light 

    // Distance attenuation (Ÿ    ġ)
    float att = saturate((light.fallOffEnd - lightDist) / (light.fallOffEnd - light.fallOffStart));

    // Shadow Map
    float shadowFactor = 1.0; // ׸ڰ ٸ ShadowFactor 1.0
    
    if (light.type & LIGHT_SHADOW) {
        uint faceIndex;
        GetCubeFace(light.type, posWorld - light.position, faceIndex);
        // Project posWorld to light screen
        //  ó ؼ Projection
        float4 lightScreen = mul(float4(posWorld, 1.0), light.viewProj[faceIndex]);
        // NDC ǥ
        lightScreen.xyz /= lightScreen.w; // Homogenization
        
        //    Texture ǥ  (Shadow Map Texture̱  ȯ)
        // [-1.0, 1.0] x [-1.0, 1.0] -> [0.0, 1.0] x [0.0, 1.0]
        // : Texture ǥ NDC y ݴ (߿)
        float2 lightTexcoord = float2(lightScreen.x, -lightScreen.y);
        lightTexcoord = (lightTexcoord + 1.0) * 0.5;
        
        //  PCSS
        shadowFactor = PCSS(lightTexcoord, lightScreen.z - 0.01, shadowIdx + faceIndex, light.invProj, light.radius, light.nearPlane, light.frustumWidth);
    }
    
    //  
    float3 radiance = light.radiance * spotFactor * att * shadowFactor;
    
    return radiance;
}

//  (point light, spot light    ߴ )
float3 DirectLighting(Light light, float3 posWorld, float3 pixelToEye, float3 normalWorld, float3 albedo, float metallic, float roughness, uint shadowIdx) {
    // Sphere Light 
    float3 L = light.position - posWorld;
    float3 r = normalize(reflect(-pixelToEye, normalWorld));
    float3 centerToRay = dot(L, r) * r - L; // Sphere Light ߽ɿ r  
    // RepresentativePoint World ǥ迡 Shading Pixel   ؼ ã
    float3 representativePoint = L + centerToRay * saturate(light.radius / length(centerToRay)); // Area Light ǥϴ  
    // posWorld ؼ World ǥ (0, 0, 0)  ǵ ȯ
    representativePoint += posWorld;
    
    float3 lightVec = representativePoint - posWorld; // Area Light 
    //float3 lightVec = light.position - posWorld; // Ϲ Light  
    
    float lightDist = length(lightVec);
    lightVec /= lightDist;
    float3 halfway = normalize(pixelToEye + lightVec);
        
    float NdotI = max(0.0, dot(normalWorld, lightVec));
    float NdotH = max(0.0, dot(normalWorld, halfway));
    float NdotO = max(0.0, dot(normalWorld, pixelToEye));
    
    float3 F0 = lerp(Fdielectric, albedo, metallic); // ü   F0 
    float3 F = SchlickFresnel(F0, max(0.0, dot(halfway, pixelToEye)));
    
    //   Ģ  ݼ ݼ  Specular Diffuse  
    //  ݼ̸ kd = 0 diffuse  Specular ,  ݼ̸ kd = 1 diffuse 
    float3 kd = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), metallic);
      
    float3 diffuseBRDF = kd * albedo;
        
    // Sphere Normalization (Sphere Light) - Area Light Specular  оǷ SphereNormalization 
    float alpha = roughness * roughness;
    float alphaPrime = saturate(alpha + light.radius / (2.0 * lightDist));
    
    float D = NdfGGX(NdotH, roughness, alphaPrime);
    float3 G = SchlickGGX(NdotI, NdotO, roughness);
    
    // 0  
    float3 specularBRDF = (F * D * G) / max(1e-5, 4.0 * NdotI * NdotO);
    
    //  
    float3 radiance = float3(0.0, 0.0, 0.0);
    radiance = LightRadiance(light, representativePoint, posWorld, normalWorld, shadowIdx);
    
    //     ϴ      ǥ ϸ  ȵ 
    return (diffuseBRDF + specularBRDF) * radiance * NdotI;
}

PSOutput main(PSInput input)
{
    float3 pixelToEye = normalize(eyeWorld - input.posWorld);
    float3 normalWorld = GetNormal(input);
    
    float maxDist = 7.0f;
    float minDist = 3.0;
    
    float dist = length(eyeWorld - input.posWorld);
    //float lod = 10.0 * saturate((dist - minDist) / (maxDist - minDist));
    float lod = 0.0;
    
    float4 albedo = useAlbedoMap ?
        albedoTex.SampleLevel(linearWrapSampler, input.texcoord, lod) * float4(albedoFactor, 1.0)
        : float4(albedoFactor, 1.0);
    
    float ao = useAOMap ? aoTex.SampleLevel(linearWrapSampler, input.texcoord, lod).r : 1.0;
    
    // Metal Texture Roughness Texture  Texture ؼ  b g  
    float metallic = useMetallicMap ? metallicTex.SampleLevel(linearWrapSampler, input.texcoord, lod).r * metallicFactor
                                    : metallicFactor;
    float roughness = useRoughnessMap ? roughnessTex.SampleLevel(linearWrapSampler, input.texcoord, lod).r * roughnessFactor
                                      : roughnessFactor;
    //float metallic = metallicFactor;
    //float roughness = roughnessFactor;
    float3 emission = useEmissiveMap ? emissiveTex.SampleLevel(linearWrapSampler, input.texcoord, lod).rgb * emissionFactor
                                     : emissionFactor;

    //  (ȯκ ޴ )
    float3 ambientLighting = AmbientLightingByIBL(albedo.rgb, normalWorld, pixelToEye, ao, metallic, roughness) * strengthIBL;
    
    //  (Direct Light) - Directional, Point, Spot Light, Sphere Light 
    float3 directLighting = float3(0.0, 0.0, 0.0);
    
    uint shadowIdx = 0;
    // ӽ÷ unroll 
    [unroll] // warning X3557: loop only executes for 1 iteration(s), forcing loop to unroll
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (!(lights[i].type & LIGHT_OFF)) {
            float3 radiance = DirectLighting(lights[i], input.posWorld, pixelToEye, normalWorld, albedo.rgb, metallic, roughness, shadowIdx);
            // TODO: radiance (0, 0, 0)  DirectLighting += ... ε direfctLight (0, 0, 0) Ǿ   ӽ 
            if (abs(dot(float3(1, 1, 1), radiance)) > 1e-5) // radiance (0, 0, 0)   
                // DirectX 11 迭 Indexing    
                // ׷ Loop ӽ÷ unroll  ( For loop Ǯ 3 ݺϴ  )
                // TODO: Texture2D ƴ Texture2DArray  ϴ   
                directLighting += radiance;
            
            if (lights[i].type & LIGHT_SHADOW)
            {
                if (lights[i].type & LIGHT_POINT)
                    shadowIdx += 6;
                else
                    shadowIdx += 1;
            }
        }
    }
    
    PSOutput output;
    // emission ü ü   κ̶   ִµ ,  ȭ   
    output.pixelColor = float4(ambientLighting + directLighting + emission, 1.0);
    output.pixelColor = clamp(output.pixelColor, 0.0, 1000.0);
    output.indexColor = HashIdToColor(hashID);
    return output;
}