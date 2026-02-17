#include "Common.hlsli"
#include "ParticleCommon.hlsli"

Texture2D albedoTex : register(t0);
Texture2D normalTex : register(t1);
Texture2D aoTex : register(t2);
Texture2D metallicTex : register(t3);
Texture2D roughnessTex : register(t4);
Texture2D emissiveTex : register(t5);
Texture2D sceneDepthTex : register(t7);

struct ParticlePSInput
{
    float4 pos : SV_POSITION;
    float4 posWorld : POSITION0;
    float4 center : POSITION1;
    float2 uv : TEXCOORD0;
    float4 color : COLOR;
    float lifeRatio : TEXCOORD1;
    uint emitterSlotID : TEXCOORD2;  // Receive from VS for sprite animation
};

// Linearize a depth value from NDC [0,1] to view-space distance (ndcDepth -> viewSpaceZ)
float LinearizeDepth(float ndcDepth) {
    // invProj from GlobalConsts b0
    // For a standard projection matrix:
    //   linearDepth = invProj._34 / (ndcDepth + invProj._33)
    return invProj._34 / (ndcDepth + invProj._33);
}

// Sprite animation texture sampling - always uses albedoTex (t0)
float4 SpriteTexture(uint emitterSlotID, float lifeRatio, float2 uv) {
    RenderConsts render = consts[emitterSlotID].render;

    // Sprite animation processing
    if (render.frameTiles.x > 1 || render.frameTiles.y > 1) {
        // Apply animation duration (0.0~1.0) - animation plays only during this portion of particle life
        float animLifeRatio = saturate(lifeRatio / render.animDuration);
        uint currentFrame = floor(animLifeRatio * render.frameCount);
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
        float4 texColor = SpriteTexture(input.emitterSlotID, input.lifeRatio, input.uv);
        finalColor *= texColor;
    }
    else
    {
        // Case 2: No texture - Default Glow Circle
        float dist = length(float2(0.5f, 0.5f) - input.uv) * 2.0f;
        float circleAlpha = saturate(1.0f - dist);

        finalColor.a *= circleAlpha;
    }

    // Soft particle fade
    float softDist = consts[input.emitterSlotID].render.softDistance;
    if (softDist > 0) {
        float2 texSize;
        sceneDepthTex.GetDimensions(texSize.x, texSize.y);
        float2 screenUV = input.pos.xy / texSize;
        float sceneDepthNDC = sceneDepthTex.SampleLevel(pointClampSampler, screenUV, 0).r;
        float softFactor = saturate((LinearizeDepth(sceneDepthNDC) - LinearizeDepth(input.pos.z)) / softDist);
        finalColor.a *= softFactor;
    }

    return finalColor;
}