#include "Common.hlsli"
#include "ParticleCommon.hlsli"

Texture2D albedoTex : register(t0);
Texture2D normalTex : register(t1);
Texture2D aoTex : register(t2);
Texture2D metallicTex : register(t3);
Texture2D roughnessTex : register(t4);
Texture2D emissiveTex : register(t5);

struct ParticlePSInput
{
    float4 pos : SV_POSITION;
    float4 posWorld : POSITION0;
    float4 center : POSITION1;
    float2 uv : TEXCOORD0;
    float4 color : COLOR;
    float lifeRatio : TEXCOORD1;
};

// Sprite animation texture sampling - always uses albedoTex (t0)
float4 SpriteTexture(float lifeRatio, float2 uv) {
    RenderConsts render = consts[emitterID].render;

    // Sprite animation processing
    if (render.frameTiles.x > 1 || render.frameTiles.y > 1) {
        uint currentFrame = floor(lifeRatio * render.frameCount);
        currentFrame = min(currentFrame, render.frameCount - 1);

        // Sprite Sheet col, row calculation
        uint width = render.frameTiles.x;
        uint col = currentFrame % width;
        uint row = currentFrame / width;

        float2 uvSize = 1.f / render.frameTiles;
        uv = (uv + float2(col, row)) * uvSize;
    }

    // Always sample from albedoTex (t0) bound by MaterialSystem
    return albedoTex.Sample(linearClampSampler, uv);
}

float4 main(ParticlePSInput input) : SV_TARGET
{
    float4 finalColor = input.color;

    // Use MaterialConstants.useAlbedoMap to determine if texture exists
    if (useAlbedoMap)
    {
        // Case 1: Texture exists (Sprite / Animation)
        float4 texColor = SpriteTexture(input.lifeRatio, input.uv);
        finalColor *= texColor;
    }
    else
    {
        // Case 2: No texture - Default Glow Circle
        float dist = length(float2(0.5f, 0.5f) - input.uv) * 2.0f;
        float circleAlpha = saturate(1.0f - dist);
        finalColor.a *= circleAlpha;
    }

    return finalColor;
}