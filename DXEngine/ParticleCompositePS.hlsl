#include "Common.hlsli"

Texture2D lowResTexture : register(t0);      // Low-res particle color (RGB) + Alpha (A)
Texture2D fullResDepth  : register(t1);      // Full-res Scene Depth (High Res)
Texture2D lowResSceneDepth : register(t2);   // Downsampled Scene Depth (Low Res) - 중요!

struct SamplingPSInput {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD;
};

float4 main(SamplingPSInput input) : SV_TARGET
{
    // 1. 현재 픽셀의 고해상도 깊이 (기준값)
    float fullDepth = fullResDepth.SampleLevel(pointClampSampler, input.texcoord, 0).r;

    // 2. 저해상도 텍스처 정보
    float2 lowResSize;
    lowResTexture.GetDimensions(lowResSize.x, lowResSize.y);
    float2 texelSize = 1.0 / lowResSize;

    // 3. 현재 UV가 저해상도 그리드에서 어디에 위치하는지 계산
    // 예: 10.4 -> 10번 픽셀과 11번 픽셀 사이, 10번에서 0.4만큼 떨어짐
    float2 lowResCoord = input.texcoord * lowResSize;
    float2 centerPos = floor(lowResCoord - 0.5) + 0.5; // 2x2 그리드의 좌상단 픽셀 중심점

    // 4. 현재 픽셀이 2x2 그리드 내에서 어디에 치우쳐져 있는지 (0.0 ~ 1.0)
    // 이것이 바로 Bilinear Interpolation의 핵심 가중치입니다.
    float2 t = lowResCoord - centerPos;

    float4 totalColor = 0;
    float totalWeight = 0.0;
    const float depthThreshold = 0.02; // 깊이 민감도 (상황에 따라 조절)

    // 2x2 수동 샘플링 루프
    [unroll]
    for (int x = 0; x <= 1; x++)
    {
        [unroll]
        for (int y = 0; y <= 1; y++)
        {
            float2 offset = float2(x, y);

            // 샘플링할 UV 좌표 (정확히 텍셀의 중심을 찍어야 함)
            float2 sampleUV = (centerPos + offset) * texelSize;

            // [중요] 수동 보간을 하므로 여기서는 Point Sampler를 써야 정확합니다.
            float lowDepth = lowResSceneDepth.SampleLevel(pointClampSampler, sampleUV, 0).r;
            float4 sampleColor = lowResTexture.SampleLevel(pointClampSampler, sampleUV, 0);

            // (A) Depth Weight: 깊이 차이에 따른 가중치
            float depthDiff = abs(fullDepth - lowDepth);
            float depthWeight = 1.0 / (1.0 + depthDiff / depthThreshold); // 깊을수록 가중치 하락

            // (B) Spatial Weight: 거리에 따른 가중치 (이게 추가됨!)
            // x=0일 때는 (1-t.x), x=1일 때는 t.x를 사용
            float spatialWeightX = (x == 0) ? (1.0 - t.x) : t.x;
            float spatialWeightY = (y == 0) ? (1.0 - t.y) : t.y;
            float spatialWeight = spatialWeightX * spatialWeightY;

            // (C) Alpha Weight: 파티클이 없는 빈 공간(0)이 색을 오염시키지 않도록 방지
            // Premultiplied Alpha를 가정한다면 sampleColor.a가 이미 rgb에 곱해져 있으니 주의 필요.
            // 보통은 그냥 sampleColor.a를 가중치로 한 번 더 곱해주는 게 Halo(후광) 제거에 좋습니다.
            float alphaWeight = sampleColor.a;

            // 최종 가중치 결합
            float weight = depthWeight * spatialWeight * alphaWeight;

            totalColor += sampleColor * weight;
            totalWeight += weight;
        }
    }

    // 결과 정규화
    float4 finalColor;
    if (totalWeight > 0.0001)
    {
        finalColor = totalColor / totalWeight;
    }
    else
    {
        // Fallback: 깊이 차이가 너무 커서 매칭되는게 없거나 알파가 다 0인 경우
        // 그냥 Bilinear (부드럽게 뭉개기)
        finalColor = lowResTexture.SampleLevel(linearClampSampler, input.texcoord, 0);
    }

    return finalColor;
}