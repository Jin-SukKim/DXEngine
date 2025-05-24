#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__

#define MAX_LIGHTS 3
#define LIGHT_OFF 0x00
#define LIGHT_DIRECTIONAL 0x01
#define LIGHT_POINT 0x02
#define LIGHT_SPOT 0x04
#define LIGHT_SHADOW 0x10

struct Light {
    float3 radiance; // ���� ���� (Strength)
    float fallOffStart; // ���� ������ �������� �����ϴ� �Ÿ� (point/spot light only)
    float3 direction; // ���� ���� (spot light only)
    float fallOffEnd; // ���� ���̻� ���� �ʾ� ��ο����� �Ÿ� (point/spot light only)
    float3 position; // ���� ��ġ (point/spot light only)
    float spotPower; // ���� �� ������ ���̴� ���� (spot light only)

	// Light type bitmasking
    uint type;
    float radius; // ������ (Volume Light ��)
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
    matrix worldIT; // Normal ��ȯ�� inverse Transpose
};

struct VSInput {
    float3 posModel : POSITION; // �� ��ǥ���� ��ġ
    float3 normalModel : NORMAL; // �� ��ǥ���� normal
    float2 texcoord : TEXCOORD;
};

struct PSInput {
    float4 posProj : SV_POSITION; // Screen ��ǥ���� ��ġ
    float3 posWorld : POSITION; // World ��ǥ���� ��ġ (���� ��꿡 ���)
    float3 normalWorld : NORMAL;
    float2 texcoord : TEXCOORD;
};

#endif // __COMMON_HLSLI__