#ifndef __BLINN_PHONG__
#define __BLINN_PHONG__

#include "Common.hlsli"

float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye) {
    // Diffuse 계산
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 diffuseTerm = diffuse * ndotl;
    
    // Specular 계산
    float3 halfway = normalize(toEye + lightVec);
    float hdotn = dot(halfway, normal);
    float3 specularTerm = specular * pow(max(hdotn, 0.0f), shininess);
    
    // 최종 조합
    return ambient + (diffuseTerm + specularTerm) * lightStrength;
}

float3 ComputeDirectionalLight(Light L, float3 normal, float3 toEye) {
    // Shading 계산을 위해선 조명을 향한 방향 벡터가 필요
    float3 lightVec = -L.direction; // Directional Light은 물체의 위치에 따라서 빛의 방향이 바뀌지 않음
    
    float ndotl = max(dot(lightVec, normal), 0.0);
    float3 lightStrength = L.radiance * ndotl;
    
    return BlinnPhong(lightStrength, lightVec, normal, toEye);
}

// 조명과 거리에 따라 빛의 강도를 조절
float CalcAttenuation(float d, float fallOffStart, float fallOffEnd) {
    // Linear Falloff (saturate = 값의 범위를 [0.0, 1.0]으로 고정
    return saturate((fallOffEnd - d) / (fallOffEnd - fallOffStart));
}

float3 ComputePointLight(Light L, float3 pos, float3 normal, float3 toEye) {
    // 물체에서 빛을 향하는 방향 벡터
    float3 lightVec = L.position - pos; // 물체의 위치에 따라 빛의 방향이 바뀜
    
    // shading할 지점부터 조명까지의 거리 계산
    float d = length(lightVec);
    
    // 너무 멀면 조명이 적용되지 않음 (Attenuation)
    if (d > L.fallOffEnd)
        return float3(0.0, 0.0, 0.0);
    else {
           // 조금이라도 불필요한 연산을 줄이기 위해 나중에 d를 나눠 Normalization
        lightVec /= d;
    
        // 표면과 광원의 각도
        float ndotl = max(dot(lightVec, normal), 0.0);
        float3 lightStrength = L.radiance * ndotl; // 각도에 따라 표면에 닿는 빛의 강도가 다름
    
        // 거리에 따른 빛의 강도 가중치
        float att = CalcAttenuation(d, L.fallOffStart, L.fallOffEnd);
        lightStrength *= att;
    
        // Blinn-Phong
        return BlinnPhong(lightStrength, lightVec, normal, toEye);
    }

}

float3 ComputeSpotLight(Light L, float3 pos, float3 normal, float3 toEye) {
    // 물체에서 빛을 향하는 방향 벡터
    float3 lightVec = L.position - pos; // 물체의 위치에 따라 빛의 방향이 바뀜
    
    // shading할 지점부터 조명까지의 거리 계산
    float d = length(lightVec);
    
    // 너무 멀면 조명이 적용되지 않음 (Attenuation)
    if (d > L.fallOffEnd)
        return float3(0.0, 0.0, 0.0);
    else {
        // 조금이라도 불필요한 연산을 줄이기 위해 나중에 d를 나눠 Normalization
        lightVec /= d;
    
        // 표면과 광원의 각도
        float ndotl = max(dot(lightVec, normal), 0.0);
        float3 lightStrength = L.radiance * ndotl; // 각도에 따라 표면에 닿는 빛의 강도가 다름
    
        // 거리에 따른 빛의 강도 가중치
        float att = CalcAttenuation(d, L.fallOffStart, L.fallOffEnd);
        lightStrength *= att;
    
        // 빛이 향하는 방향으로 빛을 모아주는 강도
        float spotFactor = pow(max(dot(L.direction, -lightVec), 0.0), L.spotPower);
        lightStrength *= spotFactor;
    
        // Blinn-Phong
        return BlinnPhong(lightStrength, lightVec, normal, toEye);
    }
}

#endif