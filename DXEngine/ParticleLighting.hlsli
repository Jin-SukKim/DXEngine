#ifndef __PARTICLE_LIGHTING_HLSLI__
#define __PARTICLE_LIGHTING_HLSLI__

// Shadow-free PBR lighting for particles
// Based on UnrealPBR.hlsl but with all shadow-related code removed
// (PCSS 64 disk samples are too expensive for particle overdraw)

static const float3 Fdielectric = 0.04;

float3 SchlickFresnelPBR(float3 F0, float NdotH) {
    return F0 + (1.0 - F0) * pow(2.0, (-5.55473 * NdotH - 6.98316) * NdotH);
}

float3 DiffuseIBL(float3 albedo, float3 normalWorld, float3 pixelToEye, float metallic) {
    float3 F0 = lerp(Fdielectric, albedo, metallic);
    float3 F = SchlickFresnelPBR(F0, max(0.0, dot(normalWorld, pixelToEye)));
    float3 kd = lerp(1.0 - F, 0.0, metallic);

    float3 irradiance = irradianceIBLTex.SampleLevel(linearWrapSampler, normalWorld, 0.0).rgb;

    return kd * albedo * irradiance;
}

float3 SpecularIBL(float3 albedo, float3 normalWorld, float3 pixelToEye, float metallic, float roughness) {
    float2 specularBRDF = brdfTex.SampleLevel(linearClampSampler, float2(dot(normalWorld, pixelToEye), 1.0 - roughness), 0.0f).rg;
    float3 specularIrradiance = specularIBLTex.SampleLevel(linearWrapSampler, reflect(-pixelToEye, normalWorld), roughness * 5.0f).rgb;
    float3 F0 = lerp(Fdielectric, albedo, metallic);

    return (F0 * specularBRDF.x + specularBRDF.y) * specularIrradiance;
}

float3 AmbientLightingByIBL(float3 albedo, float3 normalWorld, float3 pixelToEye, float ao, float metallic, float roughness) {
    float3 diffuseIBL = DiffuseIBL(albedo, normalWorld, pixelToEye, metallic);
    float3 specularIBL = SpecularIBL(albedo, normalWorld, pixelToEye, metallic, roughness);

    return (diffuseIBL + specularIBL) * ao;
}

// GGX/Towbridge-Reitz normal distribution function.
float NdfGGX(float NdotH, float roughness, float alphaPrime) {
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;
    float denom = (NdotH * NdotH) * (alphaSq - 1.0) + 1.0;

    return alphaPrime * alphaPrime / (3.141592 * denom * denom);
}

float SchlickG1(float NdotV, float k) {
    return NdotV / (NdotV * (1.0 - k) + k);
}

float SchlickGGX(float NdotI, float NdotO, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return SchlickG1(NdotI, k) * SchlickG1(NdotO, k);
}

// Shadow-free version: spotFactor + distance attenuation only
float3 LightRadiance(Light light, float3 representativePoint, float3 posWorld, float3 normalWorld) {
    float3 lightVec = light.type & LIGHT_DIRECTIONAL ?
                        -light.direction :
                        representativePoint - posWorld;

    float lightDist = length(lightVec);
    lightVec /= lightDist;

    float spotFactor = light.type & LIGHT_SPOT ?
                        pow(max(-dot(lightVec, light.direction), 0.0), light.spotPower) :
                        1.0;

    float att = saturate((light.fallOffEnd - lightDist) / (light.fallOffEnd - light.fallOffStart));

    float3 radiance = light.radiance * spotFactor * att;

    return radiance;
}

// Shadow-free Direct Lighting with Sphere Light
float3 DirectLighting(Light light, float3 posWorld, float3 pixelToEye, float3 normalWorld, float3 albedo, float metallic, float roughness) {
    // Sphere Light
    float3 L = light.position - posWorld;
    float3 r = normalize(reflect(-pixelToEye, normalWorld));
    float3 centerToRay = dot(L, r) * r - L;
    float3 representativePoint = L + centerToRay * saturate(light.radius / length(centerToRay));
    representativePoint += posWorld;

    float3 lightVec = representativePoint - posWorld;

    float lightDist = length(lightVec);
    lightVec /= lightDist;
    float3 halfway = normalize(pixelToEye + lightVec);

    float NdotI = max(0.0, dot(normalWorld, lightVec));
    float NdotH = max(0.0, dot(normalWorld, halfway));
    float NdotO = max(0.0, dot(normalWorld, pixelToEye));

    float3 F0 = lerp(Fdielectric, albedo, metallic);
    float3 F = SchlickFresnelPBR(F0, max(0.0, dot(halfway, pixelToEye)));

    float3 kd = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), metallic);

    float3 diffuseBRDF = kd * albedo;

    // Sphere Normalization
    float alpha = roughness * roughness;
    float alphaPrime = saturate(alpha + light.radius / (2.0 * lightDist));

    float D = NdfGGX(NdotH, roughness, alphaPrime);
    float3 G = SchlickGGX(NdotI, NdotO, roughness);

    float3 specularBRDF = (F * D * G) / max(1e-5, 4.0 * NdotI * NdotO);

    float3 radiance = LightRadiance(light, representativePoint, posWorld, normalWorld);

    return (diffuseBRDF + specularBRDF) * radiance * NdotI;
}

#endif // __PARTICLE_LIGHTING_HLSLI__
