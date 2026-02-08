#ifndef __FRUSTUM_CULLING_COMMON_HLSLI__
#define __FRUSTUM_CULLING_COMMON_HLSLI__

struct FrustumPlanes
{
    float4 planes[6]; // Left, Right, Bottom, Top, Near, Far (Ax+By+Cz+D=0)
};

struct FrustumCullingConsts
{
    FrustumPlanes frustum;
    matrix viewMatrix;
    uint enableCulling;
    float3 padding;
};

#endif // __FRUSTUM_CULLING_COMMON_HLSLI__
