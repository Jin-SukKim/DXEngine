#include "Common.hlsli"
#include "ParticleCommon.hlsli"

Texture2D albedoTex : register(t0);
Texture2D normalTex : register(t1);
Texture2D aoTex : register(t2);
Texture2D metallicTex : register(t3);
Texture2D roughnessTex : register(t4);
Texture2D emissiveTex : register(t5);

Texture2D singleTex : register(t6);

struct ParticlePSInput
{
    float4 pos : SV_POSITION;
    float4 posWorld : POSITION0;
    float4 center : POSITION1;
    float2 uv : TEXCOORD0;
    float4 color : COLOR;
    float lifeRatio : TEXCOORD1;
    uint primID : SV_PrimitiveID;
};

float4 SampleParticleTexture(float3 uvw)
{
    float4 color = float4(1, 1, 1, 1);

    // [Mode 0: Material]
    if (render.textureMode == 0)
    {
        // Albedo 샘플링
        color = albedoTex.Sample(linearClampSampler, uvw.xy);

        // Emissive 추가 (선택 사항 - 파티클은 주로 Emissive 속성이 강하므로 더해주는 경우가 많음)
        // float4 emissive = materialEmissiveMap.SampleLevel(samp, uvw.xy, lod);
        // color.rgb += emissive.rgb; 
    }
    // [Mode 1: Single Texture]
    else if (render.textureMode == 1)
    {
        // 개별 텍스처 샘플링 (uvw.z 인덱스 무시)
        color = singleTex.Sample(linearClampSampler, uvw.xy);
    }
    // [Mode 2: Texture Array (Default)]
    else
    {
        // 텍스처 배열 샘플링 (uvw.z = Array Index)
        color = particleTex.Sample(linearClampSampler, uvw);
    }

    return color;
}

float4 SpriteTexture(float lifeRatio, float2 uv) {
    if (render.frameTiles.x > 1 || render.frameTiles.y > 1) {
        // 현재 frame
        uint currentFrame = floor(lifeRatio * render.frameCount);
        currentFrame = min(currentFrame, render.frameCount - 1);

        // Sprite Sheet의 col, row
        uint width = render.frameTiles.x;
        uint col = currentFrame % width;
        uint row = currentFrame / width;

        float2 uvSize = 1.f / render.frameTiles; // Tile 1칸의 크기

        uv = (uv + float2(col, row)) * uvSize;
    }

    return SampleParticleTexture(float3(uv, render.textureIdx));
}

float4 main(ParticlePSInput input) : SV_TARGET
{
    float4 finalColor = input.color;

    bool hasTexture = (render.textureMode != 2) || (render.textureIdx >= 0);
    // --------------------------------------------------------
    // Case 1: 텍스처가 있는 경우 (Sprite / Animation)
    // --------------------------------------------------------
    if (hasTexture)
    {
        float4 texColor = SpriteTexture(input.lifeRatio, input.uv);
        finalColor *= texColor;

        // 텍스처의 알파가 너무 낮으면 그리지 않음 (Alpha Test)
        // 불투명/반투명 섞어 쓸 때 유용
        if (finalColor.a <= 0.01f)
            discard;
    }
    // --------------------------------------------------------
    // Case 2: 텍스처가 없는 경우 (기본 원형 Glow)
    // --------------------------------------------------------
    else
    {
        // 텍스처가 없을 때만 절차적으로 원을 그립니다.
        float dist = length(float2(0.5f, 0.5f) - input.uv) * 2.0f;
        float circleAlpha = saturate(1.0f - dist);

        // 원 밖은 잘라냄
        if (circleAlpha <= 0.0f)
            discard;

        finalColor.a *= circleAlpha;
    }

    return finalColor;
}