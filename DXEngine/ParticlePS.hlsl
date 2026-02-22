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
    float lifeMax : TEXCOORD3;
};

// Linearize a depth value from NDC [0,1] to view-space distance (ndcDepth -> viewSpaceZ)
float LinearizeDepth(float ndcDepth) {
    // invProj from GlobalConsts b0
    // For a standard projection matrix:
    //   linearDepth = invProj._34 / (ndcDepth + invProj._33)
    return invProj._34 / (ndcDepth + invProj._33);
}

// Compute sprite-animated UV for a discrete (non-blended) frame
float2 GetSpriteAnimatedUV(uint emitterSlotID, float lifeRatio, float lifeMax, float2 uv) {
    RenderConsts render = consts[emitterSlotID].render;

    if (render.frameTiles.x > 1 || render.frameTiles.y > 1) {
        float animLifeRatio;
        if (render.animTime > 0) {
            float elapsed = lifeRatio * lifeMax;
            animLifeRatio = saturate(elapsed / render.animTime);
        } else {
            animLifeRatio = saturate(lifeRatio / render.animDuration);
        }
        float2 uvSize = 1.f / render.frameTiles;
        uint width = render.frameTiles.x;

        uint currentFrame = floor(animLifeRatio * render.frameCount);
        currentFrame = min(currentFrame, render.frameCount - 1);
        uint col = currentFrame % width;
        uint row = currentFrame / width;
        uv = (uv + float2(col, row)) * uvSize;
    }

    return uv;
}

// Sprite animation texture sampling - always uses albedoTex (t0)
float4 SpriteTexture(uint emitterSlotID, float lifeRatio, float lifeMax, float2 uv) {
    RenderConsts render = consts[emitterSlotID].render;

    // Sprite animation processing
    if (render.frameTiles.x > 1 || render.frameTiles.y > 1) {
        float animLifeRatio;
        if (render.animTime > 0) {
            // Absolute time mode: animation completes in animTime seconds
            float elapsed = lifeRatio * lifeMax;
            animLifeRatio = saturate(elapsed / render.animTime);
        } else {
            // Ratio mode: animation duration as ratio of particle lifetime
            animLifeRatio = saturate(lifeRatio / render.animDuration);
        }
        float2 uvSize = 1.f / render.frameTiles;
        uint width = render.frameTiles.x;

        if (render.frameBlending) {
            // Smooth interpolation mode
            float frameFloat = animLifeRatio * render.frameCount;
            uint frame0 = min((uint)floor(frameFloat), render.frameCount - 1);
            uint frame1 = min(frame0 + 1, render.frameCount - 1);
            float blend = frac(frameFloat);

            float2 uv0 = (uv + float2(frame0 % width, frame0 / width)) * uvSize;
            float2 uv1 = (uv + float2(frame1 % width, frame1 / width)) * uvSize;

            return lerp(
                albedoTex.Sample(linearClampSampler, uv0),
                albedoTex.Sample(linearClampSampler, uv1),
                blend
            );
        } else {
            // Discrete frame mode - delegate to shared UV function
            return albedoTex.Sample(linearClampSampler, GetSpriteAnimatedUV(emitterSlotID, lifeRatio, lifeMax, uv));
        }
    }

    // Always sample from albedoTex (t0) bound by MaterialSystem
    return albedoTex.Sample(linearClampSampler, uv);
}

float4 main(ParticlePSInput input) : SV_TARGET
{
    float4 finalColor = input.color;

    float2 uv = input.uv;
    
    // UV Distortion
    RenderConsts render = consts[input.emitterSlotID].render;
    if (render.uvDistortEnabled) {
        float2 noise = SampleCurlNoise2D(uv, render.uvDistortFrequency, render.uvDistortScroll, frameConsts[input.emitterSlotID].time);
        uv += noise * render.uvDistortStrength;
    }

    // Use MaterialConstants.useAlbedoMap to determine if texture exists
    if (useAlbedoMap)
    {
        // Case 1: Texture exists (Sprite / Animation)
        float4 texColor = SpriteTexture(input.emitterSlotID, input.lifeRatio, input.lifeMax, uv);
        finalColor *= texColor;
    }
    else
    {
        // Case 2: No texture
        float dist = length(float2(0.5f, 0.5f) - uv) * 2.0f;
        if (render.solidCircle) {
            // Solid Circle: discard outside, keep alpha uniform
            clip(1.0f - dist);
        } else {
            // Glow Circle: smooth gradient falloff
            float circleAlpha = saturate(1.0f - dist);
            finalColor.a *= circleAlpha;
        }

        // Center White: lerp towards white, intensity controls spread
        // intensity 0 -> exponent 8 (tiny white dot), intensity 1 -> exponent 0.5 (wide spread)
        if (render.centerWhiteIntensity > 0.0f) {
            float gradient = saturate(1.0f - dist);
            float exponent = lerp(8.0f, 0.5f, render.centerWhiteIntensity);
            gradient = pow(gradient, exponent);
            finalColor.rgb = lerp(finalColor.rgb, float3(1,1,1), gradient);
        }
    }

    // Apply albedoFactor
    finalColor.rgb *= albedoFactor;

    // Compute sprite-animated UV for PBR textures (discrete frame, no blending)
    float2 spriteUV = GetSpriteAnimatedUV(input.emitterSlotID, input.lifeRatio, input.lifeMax, uv);

    // AO: darken based on ambient occlusion
    float ao = useAOMap ? aoTex.Sample(linearClampSampler, spriteUV).r : 1.0;
    finalColor.rgb *= ao;

    // Emissive: additive self-illumination
    float3 emission = useEmissiveMap ? emissiveTex.Sample(linearClampSampler, spriteUV).rgb * emissionFactor : emissionFactor;
    finalColor.rgb += emission;

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

    // Alpha clip (Masked blend mode)
    if (render.alphaClipThreshold > 0.0f)
        clip(finalColor.a - render.alphaClipThreshold);

    return finalColor;
}