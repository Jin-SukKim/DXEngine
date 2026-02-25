// PrepareSortDispatchCS.hlsl
// GPU-driven sort dispatch preparation
// Reads batchSortParams, computes sort parameters and indirect dispatch args
// Eliminates GPU->CPU readback stall and per-dispatch CB Map/Unmap

#include "ParticleCommon.hlsli"

#define BLOCK_SIZE 2048

cbuffer PrepareSortConsts : register(b0) {
    uint firstBatchIdx;
    uint lastBatchIdx;
    uint numSortSteps;
    uint sortParamsSlot;
    float3 cameraForward;
    float padding;
};

StructuredBuffer<BatchSortParam> batchSortParams : register(t0);
StructuredBuffer<SortStepGPU> sortStepTable : register(t1);

RWStructuredBuffer<GPUSortParams> gpuSortParams : register(u0);
RWBuffer<uint> dispatchArgs : register(u1);

uint NextPowerOf2(uint v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

[numthreads(1, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    // 1. Aggregate batch sort params
    uint totalParticleCount = 0;
    uint baseOffset = 0xFFFFFFFF;

    for (uint b = firstBatchIdx; b <= lastBatchIdx; ++b) {
        BatchSortParam param = batchSortParams[b];
        if (param.particleCount > 0) {
            if (baseOffset == 0xFFFFFFFF)
                baseOffset = param.baseOffset;
            totalParticleCount += param.particleCount;
        }
    }

    // Handle zero particles case
    if (totalParticleCount == 0) {
        // GenKeys: no-op
        dispatchArgs[0] = 0;
        dispatchArgs[1] = 1;
        dispatchArgs[2] = 1;

        // All sort steps: no-op
        for (uint i = 0; i < numSortSteps; ++i) {
            uint offset = (i + 1) * 3;
            dispatchArgs[offset]     = 0;
            dispatchArgs[offset + 1] = 1;
            dispatchArgs[offset + 2] = 1;
        }

        // CopyBack: no-op
        uint copyOffset = (numSortSteps + 1) * 3;
        dispatchArgs[copyOffset]     = 0;
        dispatchArgs[copyOffset + 1] = 1;
        dispatchArgs[copyOffset + 2] = 1;

        // Write empty sort params
        GPUSortParams sp;
        sp.sortBaseOffset = 0;
        sp.sortParticleCount = 0;
        sp.firstBatchIdx = firstBatchIdx;
        sp.lastBatchIdx = lastBatchIdx;
        sp.cameraForward = cameraForward;
        sp.sortSize = 0;
        gpuSortParams[sortParamsSlot] = sp;
        return;
    }

    // 2. Compute sort size (power of 2, at least BLOCK_SIZE)
    uint sortSize = max(NextPowerOf2(totalParticleCount), BLOCK_SIZE);

    // 3. GenKeys dispatch args
    dispatchArgs[0] = (sortSize + 1023) / 1024;
    dispatchArgs[1] = 1;
    dispatchArgs[2] = 1;

    // 4. Sort step dispatch args
    for (uint i = 0; i < numSortSteps; ++i) {
        SortStepGPU step = sortStepTable[i];
        uint offset = (i + 1) * 3;

        if (sortSize >= step.minSortSize) {
            uint groups = 0;
            if (step.phase == 1 || step.phase == 3) {
                // Phase 1 (BlockSort) / Phase 3 (InnerSort): groups = sortSize / BLOCK_SIZE
                groups = sortSize / BLOCK_SIZE;
            } else {
                // Phase 2 (GlobalSort): groups = (sortSize + 1023) / 1024
                groups = (sortSize + 1023) / 1024;
            }
            dispatchArgs[offset]     = groups;
            dispatchArgs[offset + 1] = 1;
            dispatchArgs[offset + 2] = 1;
        } else {
            dispatchArgs[offset]     = 0;
            dispatchArgs[offset + 1] = 1;
            dispatchArgs[offset + 2] = 1;
        }
    }

    // 5. CopyBack dispatch args
    uint copyOffset = (numSortSteps + 1) * 3;
    dispatchArgs[copyOffset]     = (totalParticleCount + 1023) / 1024;
    dispatchArgs[copyOffset + 1] = 1;
    dispatchArgs[copyOffset + 2] = 1;

    // 6. Write GPU sort params
    GPUSortParams sp;
    sp.sortBaseOffset = baseOffset;
    sp.sortParticleCount = totalParticleCount;
    sp.firstBatchIdx = firstBatchIdx;
    sp.lastBatchIdx = lastBatchIdx;
    sp.cameraForward = cameraForward;
    sp.sortSize = sortSize;
    gpuSortParams[sortParamsSlot] = sp;
}
