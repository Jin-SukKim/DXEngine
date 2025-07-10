// Depth Map을 렌더링
#include "Common.hlsli"

Texture2D curFrame : register(t0);
Texture2D depthOnly : register(t1); // NDC 좌표계에서의 깊이값

cbuffer DepthConsts : register(b5) {
    float depthScale;
    float3 dummy6;
};

struct SamplingPSInput {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD;
};

// Texcoord를 View 좌표계로 변환 (View 좌표계의 깊이값, 즉 카메라로부터 깊이값을 구하기 위함)
float4 TexcoordToView(float2 texcoord) {
    float4 posProj;
    
    // [0, 1] x [0, 1] -> [-1, 1] x [-1, 1] 범위로 변환 (Texture 좌표계 -> NDC(Porjection))
    posProj.xy = texcoord * 2.0 - 1.0;
    posProj.y *= -1; // 주의: y 방향을 뒤집어줘야 함 (Texture 좌표계에서 y 방향이 아래로 향하기 때문)
    posProj.z = depthOnly.Sample(linearClampSampler, texcoord).r; // 측정된 깊이값
    posProj.w = 1.0;

    // Projection -> ViewSpace
    float4 posView = mul(posProj, invProj);
    posView.xyz /= posView.w; // Homogenization
    
    return posView;
}

float4 main(SamplingPSInput input) : SV_TARGET {
    // View 좌표계에서의 깊이값을 구하기
    float z = TexcoordToView(input.texcoord).z * depthScale;
    return float4(z, z, z, 1.0); // 깊이값을 색으로 렌더링 ([검은색, 흰색] 색상 범위)
}