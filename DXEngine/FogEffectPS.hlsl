// Depth Map을 활용한 Fog 효과
#include "Common.hlsli"

Texture2D curFrame : register(t0);
Texture2D depthOnly : register(t1); // NDC 좌표계에서의 깊이값

cbuffer DepthConsts : register(b5) {
    float depthScale;
    float3 fogColor;
    float fogStrength;
    float fogMin;
    float fogMax;
    float dummy6;
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

float4 SimpleFog(float2 texcoord) {
    float4 posView = TexcoordToView(texcoord);
    // 눈(Camera)로부터 길이
    float dist = length(posView.xyz); // 눈(Camera)의 위치가 원점인 view 좌표계
    
    // 거리에 따른 강도를 0.0 ~ 1.0으로 조절할 수 있도록 계산
    float distFog = saturate((dist - fogMin) / (fogMax - fogMin) * fogStrength);
    
    // 현재 Frame
    float3 color = curFrame.Sample(linearClampSampler, texcoord).rgb;
    
    // distFog 비율에 맞춰 0.0에 가까우면 원래 색을 표현, 1.0에 가까울수록 안개때문에 안보이는 것
    color = lerp(color, fogColor, distFog);
    
    return float4(color, 1.0);
}

float4 main(SamplingPSInput input) : SV_TARGET {
    //return SimpleFog(input.texcoord); // 가장 간단하게 구현
    
    // Beer-Lambert Law로 물리현상에 가깝게 Fog 구현
    float4 posView = TexcoordToView(input.texcoord);
    float dist = length(posView.xyz); // 눈의 위치가 원점인 좌표계
       
	// 거리에 따른 강도를 0.0 ~ 1.0으로 조절할 수 있도록 계산
    float distFog = saturate((dist - fogMin) / (fogMax - fogMin));
    float fogFactor = exp(-distFog * fogStrength); // 좀 더 사실적인 안개 효과를 위해 Beer-Lambert's Law 계산
        
    float3 color = curFrame.Sample(linearClampSampler, input.texcoord).rgb;
        
    // lerp(...) = color * fogFactor + fogColor * (1 - fogFactor);
    color = lerp(fogColor, color, fogFactor);
        
    //return SimpleFog(input.texcoord);
    return float4(color, 1.0);

}