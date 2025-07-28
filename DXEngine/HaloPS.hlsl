// Depth Map을 활용한 Halo 효과
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

// 직선과 구의 충돌
int RaySphereIntersection(float3 start, float3 dir, float3 center, float radius, out float t1, out float t2) {
    float3 p = start - center;
    // start를 픽셀좌표가 아닌 원점을 사용하고 있기 때문에 dir과 p는 서로 반대 방향을 가르켜야 앞에 Halo가 있는 것
    float pdotv = dot(p, dir);
    float p2 = dot(p, p);
    float r2 = radius * radius;
    float m = pdotv * pdotv - (p2 - r2);
    
    // 충돌하지 않거나 충돌 위치가 시점의 뒤일때
    if (m < 0.0 || pdotv > 0.0) {
        t1 = 0.0;
        t2 = 0.0;
        return 0;
    }
    else {
        m = sqrt(m);
        // 충돌 지점까지의 거리
        t1 = -pdotv - m;
        t2 = -pdotv + m;
        return 1;
    }
}

// "Foundations of Game Engine Development" by Eric Lengyel, V2 p319
float HaloEmission(float3 posView, float radius, float3 lightPos) {
    // Halo
    float3 rayStart = float3(0, 0, 0); // View space (시점의 위치가 원점)
    float3 dir = normalize(posView - rayStart); // Shading 지점을 향하는 방향 벡터
    
    // 구의 중심은 Light의 Pos에 View 행렬을 곱해 View 좌표계로 변환해서 사용
    float3 center = mul(float4(lightPos, 1.0), view).xyz; // View 공간으로 변환

    float t1 = 0.0; // 충돌 시작점
    float t2 = 0.0; // 충돌 끝점
    
    // 광선이 구와 충돌하면 빛을 얼마나 모을지 계산
    //      z값을 사용해 비교하지 않고 length()를 사용한 이유 : view space가 회전된 경우, dir의 방향이 z축이라고 장담할 수 없기 떄문
    if (RaySphereIntersection(rayStart, dir, center, radius, t1, t2) && t1 < length(posView)) // length(posView) < t1 : 물체가 Halo보다 앞에 있는 경우 Halo계산이 필요없음
    {
        // Halo를 가리는 물체가 없더라도 Halo안에 있을 수도 있으니 min으로 t2 결정
        t2 = min(length(posView), t2);
        
        // Halo 영역의 빛의 합
        float3 p = rayStart - center;
        float p2 = dot(p, p);
        float pDotv = dot(p, dir);
        float v2 = 1.0; // dot(dir, dir), dir이 unit vector인 경우 값은 1.0
        float r2 = radius * radius;
        float invR2 = 1.0 / r2;
        float haloEmission = (1.0 - p2 * invR2) * (t2 - t1)
                        - pDotv * invR2 * (t2 * t2 - t1 * t1)
                        - v2 / (3.0 * r2) * (t2 * t2 * t2 - t1 * t1 * t1);

        // Normalize - 빛을 가장 많이 모을 수 있는 경로는 Halo의 중간을 관통하는 경우인데
        // 빛을 모으는 수식을 계산하면 (4 * radius / 3.0)이 나옴
        // 그래서 반지름을 조절할 때 좀 더 편하도록 빛을 가장 많이 모으는 경우의 값으로 정규화
        haloEmission /= (4 * radius / 3.0);
        
        return haloEmission;

    }
    // 물체가 Halo를 가리는 경우
    else {
        return 0.0;
    }
}

float4 main(SamplingPSInput input) : SV_TARGET {
    float3 color = curFrame.Sample(linearClampSampler, input.texcoord).rgb;
    
    float4 posView = TexcoordToView(input.texcoord);
    
    // Halo
    float3 haloColor = float3(0.96, 0.96, 0.82);
    float radius = lights[0].haloRadius;
    // 빛이 조명으로부터 반지름 거리 안에서 진행하면서 빛을 받게되면서 밝아지기 때문에 
    // 렌더링한 결과(픽셀색)에 빛의 밝기를 더해주고 있는 것 
    
    for (int i = 0; i < MAX_LIGHTS; ++i)
        color += HaloEmission(posView.xyz, radius, lights[i].position) * haloColor * lights[i].haloStrength;
    
    return float4(color, 1.0);

}