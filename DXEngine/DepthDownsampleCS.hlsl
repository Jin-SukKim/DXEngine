// DepthDownsampleCS.hlsl
// Downsamples full-resolution depth buffer to low-resolution depth buffer
// Used to provide scene depth information to low-res billboard particle rendering

Texture2D<float> fullResDepth : register(t0);
RWTexture2D<float> lowResDepth : register(u0);

cbuffer DownsampleConsts : register(b0) {
    uint2 outputSize;
    uint2 padding;
};

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 lowResCoord = dispatchThreadID.xy;

    if (lowResCoord.x >= outputSize.x || lowResCoord.y >= outputSize.y)
        return;

    // Map to full-res: each low-res pixel = 2x2 full-res block
    uint2 fullResBase = lowResCoord * 2;

    // Sample 2x2 block and take MINIMUM depth (closest surface)
    // Ensures conservative occlusion - if any pixel has closer geometry,
    // the particle will be occluded
    float depth0 = fullResDepth[fullResBase + uint2(0, 0)];
    float depth1 = fullResDepth[fullResBase + uint2(1, 0)];
    float depth2 = fullResDepth[fullResBase + uint2(0, 1)];
    float depth3 = fullResDepth[fullResBase + uint2(1, 1)];

    float minDepth = min(min(depth0, depth1), min(depth2, depth3));

    lowResDepth[lowResCoord] = minDepth;
}
