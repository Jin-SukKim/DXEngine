#include "Common.hlsli"
#include "ParticleCommon.hlsli"

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

    return particleTex.Sample(linearClampSampler, float3(uv, render.textureIdx));
}

float4 main(ParticlePSInput input) : SV_TARGET
{
    float4 finalColor = input.color;

    // --------------------------------------------------------
    // Case 1: 텍스처가 있는 경우 (Sprite / Animation)
    // --------------------------------------------------------
    if (render.textureIdx >= 0)
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