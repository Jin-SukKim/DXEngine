#include "ParticleCommon.hlsli"
#include "FrustumCullingCommon.hlsli"

cbuffer FrustumBuffer : register(b7)
{
    FrustumCullingConsts frustumConsts;
};

RWStructuredBuffer<uint> visibleIndices : register(u4);
RWStructuredBuffer<uint> visibleCount : register(u5);

bool IsPointInFrustum(float3 posWorld)
{
    // Transform to view space
    float3 posView = mul(float4(posWorld, 1.0), frustumConsts.viewMatrix).xyz;

    // Test against all 6 frustum planes
    [unroll]
    for (uint i = 0; i < 6; ++i)
    {
        float4 plane = frustumConsts.frustum.planes[i];
        // If point is on negative side of plane, it's outside
        if (dot(plane.xyz, posView) + plane.w < 0)
            return false;
    }
    return true;
}

[numthreads(1024, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    // Read particle to get its owner emitter
    Particle p = readParticles[dtID.x];

    // Get emitter info from particle's ownerID
    EmitterID eID = emitterIDs[p.ownerID];

    // Bounds check: ensure this particle belongs to this emitter's range
    if (dtID.x < eID.readParticleOffset || dtID.x >= eID.readParticleOffset + readCount[p.ownerID])
        return;

    // Calculate local particle index within this emitter
    uint localParticleIdx = dtID.x - eID.readParticleOffset;

    bool isVisible = true;

    // If culling is enabled, perform frustum test
    if (frustumConsts.enableCulling == 1)
    {
        // Perform frustum test
        if (!IsPointInFrustum(p.position))
            isVisible = false;
    }

    // If particle is visible (or culling disabled), add to visible list
    if (isVisible)
    {
        uint writeIdx;
        InterlockedAdd(visibleCount[p.ownerID], 1, writeIdx);
        // Write the local particle index to visibleIndices
        visibleIndices[eID.readParticleOffset + writeIdx] = localParticleIdx;
    }
}
