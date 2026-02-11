#include "ParticleCommon.hlsli"

RWBuffer<uint> batchBillboardArgs : register(u0);
RWStructuredBuffer<uint> emitterWriteOffsets : register(u1);

cbuffer BatchRenderArgsConsts : register(b0) {
    uint numBatches;
    uint3 batchArgsPadding;
};

// Single-threaded: compute running global offset across all batches
[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint globalOffset = 0;

    for (uint batchID = 0; batchID < numBatches; batchID++) {
        BatchDescriptor batch = batchDescriptors[batchID];

        uint totalInstances = 0;
        for (uint i = 0; i < batch.emitterCount; i++) {
            uint eid = batchEmitterList[batch.emitterListOffset + i];
            uint count = uint(float(readCount[eid]) * frameConsts[eid].spawnRatio);

            // Global write offset for each emitter (used by BuildAliveIndicesCS)
            emitterWriteOffsets[batch.emitterListOffset + i] = globalOffset + totalInstances;

            totalInstances += count;
        }

        // Write merged draw args
        uint argsIdx = batchID * 5;
        batchBillboardArgs[argsIdx + 0] = batch.indexCount;
        batchBillboardArgs[argsIdx + 1] = totalInstances;
        batchBillboardArgs[argsIdx + 2] = batch.startIndexLocation;
        batchBillboardArgs[argsIdx + 3] = batch.baseVertexLocation;
        batchBillboardArgs[argsIdx + 4] = 0;

        globalOffset += totalInstances;
    }
}
