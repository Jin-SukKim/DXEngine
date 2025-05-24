#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__

#define MAX_LIGHTS 3
#define LIGHT_OFF 0x00
#define LIGHT_DIRECTIONAL 0x01
#define LIGHT_POINT 0x02
#define LIGHT_SPOT 0x04
#define LIGHT_SHADOW 0x10

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
};

cbuffer MeshConstants : register(b2) {
    matrix world;
    matrix worldIT; // Normal 변환용 inverse Transpose
};

struct VSInput {
    float3 posModel : POSITION; // 모델 좌표계의 위치
    float3 normalModel : NORMAL; // 모델 좌표계의 normal
    float2 texcoord : TEXCOORD;
};

struct PSInput {
    float4 posProj : SV_POSITION; // Screen 좌표계의 위치
    float3 posWorld : POSITION; // World 좌표계의 위치 (조명 계산에 사용)
    float3 normalWorld : NORMAL;
    float2 texcoord : TEXCOORD;
};

#endif // __COMMON_HLSLI__