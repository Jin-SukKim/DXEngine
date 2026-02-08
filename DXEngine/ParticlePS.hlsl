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

float4 SampleParticleTexture(float3 uvw)
{
    float4 color = float4(1, 1, 1, 1);

    RenderConsts render = consts[emitterID].render;
    // [Mode 0: Material]
    if (render.textureMode == 0)
    {
        // Albedo ���ø�
        color = albedoTex.Sample(linearClampSampler, uvw.xy);

        // Emissive �߰� (���� ���� - ��ƼŬ�� �ַ� Emissive �Ӽ��� ���ϹǷ� �����ִ� ��찡 ����)
        // float4 emissive = materialEmissiveMap.SampleLevel(samp, uvw.xy, lod);
        // color.rgb += emissive.rgb; 
    }
    // [Mode 1: Single Texture]
    else if (render.textureMode == 1)
    {
        // ���� �ؽ�ó ���ø� (uvw.z �ε��� ����)
        color = albedoTex.Sample(linearClampSampler, uvw.xy);
    }
    // [Mode 2: Texture Array (Default)]
    else
    {
        // �ؽ�ó �迭 ���ø� (uvw.z = Array Index)
        color = particleTex.Sample(linearClampSampler, uvw);
    }

    return color;
}

float4 SpriteTexture(float lifeRatio, float2 uv) {
    RenderConsts render = consts[emitterID].render;
    if (render.frameTiles.x > 1 || render.frameTiles.y > 1) {
        // ���� frame
        uint currentFrame = floor(lifeRatio * render.frameCount);
        currentFrame = min(currentFrame, render.frameCount - 1);

        // Sprite Sheet�� col, row
        uint width = render.frameTiles.x;
        uint col = currentFrame % width;
        uint row = currentFrame / width;

        float2 uvSize = 1.f / render.frameTiles; // Tile 1ĭ�� ũ��

        uv = (uv + float2(col, row)) * uvSize;
    }

    return SampleParticleTexture(float3(uv, render.textureIdx));
}

float4 main(ParticlePSInput input) : SV_TARGET
{
    float4 finalColor = input.color;

    RenderConsts render = consts[emitterID].render;
    bool hasTexture = (render.textureMode != 2) || (render.textureIdx >= 0);
    // --------------------------------------------------------
    // Case 1: �ؽ�ó�� �ִ� ��� (Sprite / Animation)
    // --------------------------------------------------------
    if (hasTexture)
    {
        float4 texColor = SpriteTexture(input.lifeRatio, input.uv);
        finalColor *= texColor;

        // Additive Blend에서는 discard 제거 (성능 향상)
        // Alpha Blend가 자동으로 투명도 처리
    }
    // --------------------------------------------------------
    // Case 2: �ؽ�ó�� ���� ��� (�⺻ ���� Glow)
    // --------------------------------------------------------
    else
    {
        // �ؽ�ó�� ���� ���� ���������� ���� �׸��ϴ�.
        float dist = length(float2(0.5f, 0.5f) - input.uv) * 2.0f;
        float circleAlpha = saturate(1.0f - dist);

        // Additive Blend에서는 discard 제거 (성능 향상)
        finalColor.a *= circleAlpha;
    }

    return finalColor;
}