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
    uint primID : SV_PrimitiveID;
    nointerpolation uint emitterID : BLENDINDICES;
};

float4 SampleParticleTexture(float3 uvw, uint eid)
{
    float4 color = float4(1, 1, 1, 1);

    RenderConsts render = consts[eid].render;
    // [Mode 0: Material]
    if (render.textureMode == 0)
    {
        color = albedoTex.Sample(linearClampSampler, uvw.xy);
    }
    // [Mode 1: Single Texture]
    else if (render.textureMode == 1)
    {
        color = albedoTex.Sample(linearClampSampler, uvw.xy);
    }
    // [Mode 2: Texture Array (Default)]
    else
    {
        color = particleTex.Sample(linearClampSampler, uvw);
    }

    return color;
}

float4 SpriteTexture(float lifeRatio, float2 uv, uint eid) {
    RenderConsts render = consts[eid].render;
    if (render.frameTiles.x > 1 || render.frameTiles.y > 1) {
        uint currentFrame = floor(lifeRatio * render.frameCount);
        currentFrame = min(currentFrame, render.frameCount - 1);

        uint width = render.frameTiles.x;
        uint col = currentFrame % width;
        uint row = currentFrame / width;

        float2 uvSize = 1.f / render.frameTiles;

        uv = (uv + float2(col, row)) * uvSize;
    }

    return SampleParticleTexture(float3(uv, render.textureIdx), eid);
}

float4 main(ParticlePSInput input) : SV_TARGET
{
    float4 finalColor = input.color;

    uint eid = input.emitterID;
    RenderConsts render = consts[eid].render;
    bool hasTexture = (render.textureMode != 2) || (render.textureIdx >= 0);
    // --------------------------------------------------------
    // Case 1: Texture (Sprite / Animation)
    // --------------------------------------------------------
    if (hasTexture)
    {
        float4 texColor = SpriteTexture(input.lifeRatio, input.uv, eid);
        finalColor *= texColor;

        if (finalColor.a <= 0.01f)
            discard;
    }
    // --------------------------------------------------------
    // Case 2: No Texture (Basic Circle Glow)
    // --------------------------------------------------------
    else
    {
        float dist = length(float2(0.5f, 0.5f) - input.uv) * 2.0f;
        float circleAlpha = saturate(1.0f - dist);

        if (circleAlpha <= 0.0f)
            discard;

        finalColor.a *= circleAlpha;
    }

    return finalColor;
}
