#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__

#define MAX_LIGHTS 3
#define LIGHT_OFF 0x00
#define LIGHT_DIRECTIONAL 0x01
#define LIGHT_POINT 0x02
#define LIGHT_SPOT 0x04
#define LIGHT_SHADOW 0x10

// Sampler들은 모든 Shader에서 공통으로 사용
SamplerState linearWrapSampler : register(s0);
SamplerState linearClampSampler : register(s1);

// 공용 Texture들 t10부터 시작 (IBL용 Texture 등)
TextureCube envIBLTex : register(t10);
TextureCube specularIBLTex : register(t11);
TextureCube irradianceIBLTex : register(t12);
Texture2D brdfTex : register(t13);

struct Light {
    float3 radiance; // 빛의 세기 (Strength)
    float fallOffStart; // 빛의 강도가 약해지기 시작하는 거리 (point/spot light only)
    float3 direction; // 빛의 방향 (spot light only)
    float fallOffEnd; // 빛이 더이상 닿지 않아 어두워지는 거리 (point/spot light only)
    float3 position; // 빛의 위치 (point/spot light only)
    float spotPower; // 빛이 한 지점에 모이는 강도 (spot light only)

	// Light type bitmasking
    uint type;
    float radius; // 반지름 (Volume Light 용)
    float2 dummy;
};

cbuffer GlobalConsts : register(b0) {
    matrix view;
    matrix proj;
    matrix viewProj;
    
    float3 eyeWorld;
    float dummy;
    
    Light lights[MAX_LIGHTS];
};

cbuffer BasicMaterialConstants : register(b1) {
    float3 ambient;
    float shininess = 0.1f;
    float3 diffuse;
    float dummy1;
    float3 specular;
    float dummy2;
    float3 fresnelR0;
    float dummy3;
    int hashID;
};

cbuffer MeshConstants : register(b2) {
    matrix world;
    matrix worldIT; // Normal 변환용 inverse Transpose
    int useHeightMap;
    float heightScale;
    float2 dummy4;
};

cbuffer MaterialConstants : register(b3) {
    float3 albedoFactor; 
    float roughnessFactor;
    float metallicFactor;
    float3 emissionFactor;
    
    // 여러 옵션들에 uint32를 flag로 하나만 사용할 수도 있음
    int useAlbedoMap = 0;
    int useNormalMap = 0;
    int useAOMap = 0;
    int invertNormalMapY = 0;
    int useMetallicMap = 0;
    int useRoughnessMap = 0;
    int useEmissiveMap = 0;
    float dummy5 = 0.f;
}

struct VSInput {
    float3 posModel : POSITION; // 모델 좌표계의 위치
    float3 normalModel : NORMAL; // 모델 좌표계의 normal
    float2 texcoord : TEXCOORD;
    float3 tangentModel : TANGENT;
};

struct PSInput {
    float4 posProj : SV_POSITION; // Screen 좌표계의 위치
    float3 posWorld : POSITION; // World 좌표계의 위치 (조명 계산에 사용)
    float3 normalWorld : NORMAL;
    float2 texcoord : TEXCOORD;
    float3 tangentWorld : TANGENT;
};

struct PSOutput {
    float4 pixelColor : SV_Target0; // Default
    float4 indexColor : SV_Target1; // Mouse Picking
};

// Schlick approximation: Eq. 9.17 in "Real-Time Rendering 4th Ed."
// fresnelR0는 물질의 고유 성질
// Water : (0.02, 0.02, 0.02)
// Glass : (0.08, 0.08, 0.08)
// Plastic : (0.05, 0.05, 0.05)
// Gold: (1.0, 0.71, 0.29)
// Silver: (0.95, 0.93, 0.88)
// Copper: (0.95, 0.64, 0.54)
float3 SchlickFresnel(float3 fresnelR0, float3 normal, float3 toEye) {
    // 참고 자료들
    // THE SCHLICK FRESNEL APPROXIMATION by Zander Majercik, NVIDIA
    // http://psgraphics.blogspot.com/2020/03/fresnel-equations-schlick-approximation.html
    
    float normalDotView = saturate(dot(normal, toEye));

    float f0 = 1.0f - normalDotView; // 90도이면 f0 = 1, 0도이면 f0 = 0

    // 1.0 보다 작은 값은 여러 번 곱하면 더 작은 값이 됩니다.
    // 0도 -> f0 = 0 -> fresnelR0 반환
    // 90도 -> f0 = 1.0 -> float3(1.0) 반환
    // 0도에 가까운 가장자리는 Specular 색상, 90도에 가까운 안쪽은 고유 색상(fresnelR0)
    return fresnelR0 + (1.0f - fresnelR0) * pow(f0, 5.0);
}

float4 HashIdToColor(int hashId) {
    float4 color;
    // 0xff = 255 (8 bit)
    color[0] = ((hashID >> 16) & 0xff) / 255.0; // r
    color[1] = ((hashID >> 8) & 0xff) / 255.0; // g
    color[2] = (hashID & 0xff) / 255.0; // b
    color[3] = 1.0; // a
    
    return color;
}

#endif // __COMMON_HLSLI__