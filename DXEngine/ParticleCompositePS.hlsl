#include "Common.hlsli"


Texture2D lowResTexture : register(t0);      // Low-resolution particle color
Texture2D fullResDepth : register(t1);       // Full-resolution scene depth
Texture2D lowResDepth : register(t2);        // Low-resolution particle depth

struct SamplingPSInput {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD;
};

// Off-Screen Particle Rendering with Bilateral Upsampling
// Reference: "Particle Rendering" techniques from GPU Pro series
float4 main(SamplingPSInput input) : SV_TARGET{
    // Get full resolution depth at current pixel
    float fullDepth = fullResDepth.Sample(pointClampSampler, input.texcoord).r;
    
    float particleDepth = lowResDepth.Sample(pointClampSampler, input.texcoord).r;
    
    // Calculate low-resolution texel size
    float2 lowResTexelSize;
    lowResTexture.GetDimensions(lowResTexelSize.x, lowResTexelSize.y);
    lowResTexelSize = 1.0 / lowResTexelSize;

    // Bilateral upsampling with depth awareness
    // Sample from a 2x2 region in the low-res texture
    float4 color = float4(0, 0, 0, 0);
    float totalWeight = 0.0;

    // Depth threshold for bilateral filtering
    const float depthThreshold = 0.01;

    // Sample offsets for 2x2 bilateral filter
    const float2 offsets[4] = {
        float2(-0.25, -0.25),
        float2(0.25, -0.25),
        float2(-0.25,  0.25),
        float2(0.25,  0.25)
    };

    // Perform bilateral upsampling
    for (int i = 0; i < 4; i++)
    {
        float2 sampleUV = input.texcoord + offsets[i] * lowResTexelSize;

        // Sample low-res color and depth
        float4 sampleColor = lowResTexture.Sample(linearClampSampler, sampleUV);
        float sampleDepth = lowResDepth.Sample(pointClampSampler, sampleUV).r;

        // Calculate depth-based weight
        // If the depth difference is too large, reduce the weight to avoid bleeding
        float depthDiff = abs(sampleDepth - fullDepth);
        float depthWeight = exp(-depthDiff / depthThreshold);

        // Spatial weight (distance from current pixel)
        float2 offset = offsets[i];
        float spatialWeight = exp(-dot(offset, offset) * 2.0);

        // Combined weight
        float weight = depthWeight * spatialWeight * sampleColor.a;

        color += sampleColor * weight;
        totalWeight += weight;
    }

    // Normalize by total weight
    if (totalWeight > 0.0001)
    {
        color /= totalWeight;
    }
    else
    {
        // Fallback to simple bilinear sampling if no valid samples
        color = lowResTexture.Sample(linearClampSampler, input.texcoord);
    }

    // Soft Particles: Fade out when particle depth is close to scene depth
    // This prevents hard edges at geometry intersections
    const float softParticleDistance = 0.05; // Adjust for softer/harder transitions
    // 이 수치가 곧 "가시성(Visibility)"입니다.
    float depthDifference = fullDepth - particleDepth;
    float visibility = saturate(depthDifference / softParticleDistance);

    // [중요] 가산 혼합(Additive)이든 알파 블렌딩이든,
    // 색상 자체를 어둡게(0) 만들어야 안 보입니다.
    color.rgb *= visibility;

    // 알파 블렌딩을 위해 알파에도 곱해줍니다.
    color.a *= visibility;

    // [수정] 강제로 1.0을 넣지 말고 계산된 color를 리턴하세요.
    return color;
}