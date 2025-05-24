#ifndef __BLINN_PHONG__
#define __BLINN_PHONG__

#include "Common.hlsli"

float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye) {
    // Diffuse ���
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 diffuseTerm = diffuse * ndotl;
    
    // Specular ���
    float3 halfway = normalize(toEye + lightVec);
    float hdotn = dot(halfway, normal);
    float3 specularTerm = specular * pow(max(hdotn, 0.0f), shininess);
    
    // ���� ����
    return ambient + (diffuseTerm + specularTerm) * lightStrength;
}

float3 ComputeDirectionalLight(Light L, float3 normal, float3 toEye) {
    // Shading ����� ���ؼ� ������ ���� ���� ���Ͱ� �ʿ�
    float3 lightVec = -L.direction; // Directional Light�� ��ü�� ��ġ�� ���� ���� ������ �ٲ��� ����
    
    float ndotl = max(dot(lightVec, normal), 0.0);
    float3 lightStrength = L.radiance * ndotl;
    
    return BlinnPhong(lightStrength, lightVec, normal, toEye);
}

// ����� �Ÿ��� ���� ���� ������ ����
float CalcAttenuation(float d, float fallOffStart, float fallOffEnd) {
    // Linear Falloff (saturate = ���� ������ [0.0, 1.0]���� ����
    return saturate((fallOffEnd - d) / (fallOffEnd - fallOffStart));
}

float3 ComputePointLight(Light L, float3 pos, float3 normal, float3 toEye) {
    // ��ü���� ���� ���ϴ� ���� ����
    float3 lightVec = L.position - pos; // ��ü�� ��ġ�� ���� ���� ������ �ٲ�
    
    // shading�� �������� ��������� �Ÿ� ���
    float d = length(lightVec);
    
    // �ʹ� �ָ� ������ ������� ���� (Attenuation)
    if (d > L.fallOffEnd)
        return float3(0.0, 0.0, 0.0);
    else {
           // �����̶� ���ʿ��� ������ ���̱� ���� ���߿� d�� ���� Normalization
        lightVec /= d;
    
        // ǥ��� ������ ����
        float ndotl = max(dot(lightVec, normal), 0.0);
        float3 lightStrength = L.radiance * ndotl; // ������ ���� ǥ�鿡 ��� ���� ������ �ٸ�
    
        // �Ÿ��� ���� ���� ���� ����ġ
        float att = CalcAttenuation(d, L.fallOffStart, L.fallOffEnd);
        lightStrength *= att;
    
        // Blinn-Phong
        return BlinnPhong(lightStrength, lightVec, normal, toEye);
    }

}

float3 ComputeSpotLight(Light L, float3 pos, float3 normal, float3 toEye) {
    // ��ü���� ���� ���ϴ� ���� ����
    float3 lightVec = L.position - pos; // ��ü�� ��ġ�� ���� ���� ������ �ٲ�
    
    // shading�� �������� ��������� �Ÿ� ���
    float d = length(lightVec);
    
    // �ʹ� �ָ� ������ ������� ���� (Attenuation)
    if (d > L.fallOffEnd)
        return float3(0.0, 0.0, 0.0);
    else {
        // �����̶� ���ʿ��� ������ ���̱� ���� ���߿� d�� ���� Normalization
        lightVec /= d;
    
        // ǥ��� ������ ����
        float ndotl = max(dot(lightVec, normal), 0.0);
        float3 lightStrength = L.radiance * ndotl; // ������ ���� ǥ�鿡 ��� ���� ������ �ٸ�
    
        // �Ÿ��� ���� ���� ���� ����ġ
        float att = CalcAttenuation(d, L.fallOffStart, L.fallOffEnd);
        lightStrength *= att;
    
        // ���� ���ϴ� �������� ���� ����ִ� ����
        float spotFactor = pow(max(dot(L.direction, -lightVec), 0.0), L.spotPower);
        lightStrength *= spotFactor;
    
        // Blinn-Phong
        return BlinnPhong(lightStrength, lightVec, normal, toEye);
    }
}

#endif