#ifndef __SHADOW_HLSLI__
#define __SHADOW_HLSLI__

#include "Common.hlsli"
#include "DiskSamples.hlsli"

// TODO: Constant Buffer로 값을 받아 사용
//#define NEAR_PLANE 0.1 // Shadow Map을 만들때 사용한 Near Z값을 사용
// #define LIGHT_WORLD_RADIUS 0.001
//#define LIGHT_FRUSTUM_WIDTH 0.34641 // <- 계산해서 찾은 값

// Assuming that LIGHT_FRUSTUM_WIDTH == LIGHT_FRUSTUM_HEIGHT
// #define LIGHT_RADIUS_UV (LIGHT_WORLD_RADIUS / LIGHT_FRUSTUM_WIDTH)

float PCF_Filter(float2 uv, float zReceiverNdc, float filterRadiusUV, Texture2D shadowMap) {
    float sum = 0.0;
    for (int i = 0; i < 64; ++i) {
        // Disk안에 패턴을 생성하는 코드로 Sampling할 지점들을 생성해 저장한 배열 diskSample을 Sampling할 Point로 사용
        // (하지만 자연스럽진 않기 때문에 Noise를 활용하면 자연스러워 보임. 단, 가까이서는 노이즈가 낀것 같이 보일 수 있음)
        float2 offset = diskSamples64[i] * filterRadiusUV;
        sum += shadowMap.SampleCmpLevelZero(shadowCompare, uv + offset, zReceiverNdc);
    }
    
    return sum / 64;
}

// Ndc Depth to View Depth
float N2V(float ndcDepth, matrix invProj)
{
    float4 pointView = mul(float4(0, 0, ndcDepth, 1), invProj);
    return pointView.z / pointView.w;
}

// 더 빠른 NDC to View 변환
float FasterN2V(float z_ndc) {
    // Projection 행렬을 사용해 계싼
	// z_ndc = A + B/viewZ, where gProj[2, 2] = A and gProj[3, 2] = B
    float viewZ = proj[3][2] / (z_ndc - proj[2][2]);
    return viewZ;
}

// void Func(out float a) <- c++의 void Func(float& a) 처럼 출력값 저장 가능
// Shading하고 있는 Pixel에 그림자가 생길 가능성이 있는지 없는지 파악 - 빛을 가로막는 물체가 있는지 찾는 단계
void FindBlocker(out float avgBlockerDepthView, out float numBlockers, float2 uv,
                 float zReceiverView, Texture2D shadowMap, matrix invProj, float lightRadiusWorld, float nearPlane, float frustumWidth)
{
    
    // Texture 좌표 공간 크기에서 봤을때의 조명의 크기를 계산
    // 조명의 크기와 조명의 ViewFrustum의 넓이의 World 좌표계 기준에서의 값을 찾아
    // 두 값의 비율로 Texture 좌표계에서의 조명의 넓이를 계산
    float lightRadiusUV = lightRadiusWorld / frustumWidth; // 쉽게 생각해 조명의 Radius를 NDC 좌표계로 변환한 것
    
    // 조명의 넓이와 Near Plane 위에서의 넓이의 비율과 조명에서 Sampling 지점까지의 거리와 
    // Near Plane에서 Sampling 지점까지의 거리의 비율이 동일
    // 조명의 넓이와 Near Plane의 거리는 정의한 값으로 알고 있으니 depth값을 이용해 비율을 계산해서
    // 조명의 넓이를 곱해 Near Plane 위에서의 넓이를 계산
    float searchRadius = lightRadiusUV * (zReceiverView - nearPlane) / zReceiverView;

    float blockerSum = 0;
    numBlockers = 0;
    // 조명 넓이에서 Depth값을 모두 보고 가리는 물체가 있는지 없는지 판단
    for (int i = 0; i < 64; ++i) {
        float shadowMapDepth =
            shadowMap.SampleLevel(shadowPointSampler, float2(uv + diskSamples64[i] * searchRadius), 0).r;

        // 평균을 계산하려면 Depth값이 선형(Linear)한 공간에 있어야 하는데 NDC에서는 깊이가 비선형 공간이므로
        // NDC에서 View 좌표계로 변환해 linear한 Depth값으로 변환해서 사용
        shadowMapDepth = N2V(shadowMapDepth, invProj);
        
        if (shadowMapDepth < zReceiverView) {
            blockerSum += shadowMapDepth;
            numBlockers++;
        }
    }
    
    // 가리는 물체의 평균 Depth값
    avgBlockerDepthView = blockerSum / numBlockers;
}

//  PCSS는 조명과 물체 간의 깊이(Depth)를 가지고 PCF의 반지름을 조절하는 것이 기본적인 원리
float PCSS(float2 uv, float zReceiverNdc, Texture2D shadowMap, matrix invProj, float lightRadiusWorld, float nearPlane, float frustumWidth) {
    float lightRadiusUV = lightRadiusWorld / frustumWidth;
    
    float zReceiverView = N2V(zReceiverNdc, invProj);
    
    // STEP 1: blocker search
    float avgBlockerDepthView = 0;
    float numBlockers = 0;

    // 가리는 물체가 없는지 탐색
    FindBlocker(avgBlockerDepthView, numBlockers, uv, zReceiverView, shadowMap, invProj, lightRadiusWorld, nearPlane, frustumWidth);

    // 가리는 물체가 없다면
    if (numBlockers < 1) {
        // There are no occluders so early out(this saves filtering)
        return 1.0f;
    }
    // 가리는 물체가 있다면
    else {
        // STEP 2: penumbra size - Penumbra 크기에 따라 PCF의 반지름의 크기를 조절해 그림자 강도(Shadow Factor) 결정
        //  Penumbra가 넓은때는 넓은 반지름, 작을때는 작은 반지름 사용
        float penumbraRatio = (zReceiverView - avgBlockerDepthView) / avgBlockerDepthView; // Penumbra 비율 계싼
        // Penumbra의 넓이는 Light의 넓이와 비례하기 때문에 비율을 통해 PCF를 위한 Radius 계산
        //      penumbraSize = penumbraRatio * lightRadiusUV;
        //      Near Plane 위에서의 PenumbraSize = penumbraSize * NEAR_PLANE / zReceiverView;
        float filterRadiusUV = penumbraRatio * lightRadiusUV * nearPlane / zReceiverView;

        // STEP 3: filtering
        return PCF_Filter(uv, zReceiverNdc, filterRadiusUV, shadowMap);
    }
}

#endif