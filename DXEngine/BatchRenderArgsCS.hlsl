#include "ParticleCommon.hlsli"

RWBuffer<uint> batchBillboardArgs : register(u0);
RWStructuredBuffer<uint> emitterWriteOffsets : register(u1);
RWStructuredBuffer<BatchSortParam> batchSortParams : register(u2);

StructuredBuffer<uint> simulationAliveCount : register(t29);

cbuffer BatchRenderArgsConsts : register(b0) {
    uint numBatches;
    uint3 batchArgsPadding;
};

//   Batch drawArgs  (Single Thread)
// TODO: Multi-Thread ĥ   
[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint globalOffset = 0;

    for (uint batchID = 0; batchID < numBatches; batchID++) {
        // Batch
        BatchDescriptor batch = batchDescriptors[batchID];

        // Batch  Emitter id  (̰  AliveIndices ä)
        uint totalInstances = 0;
        for (uint i = 0; i < batch.emitterCount; i++) {
            uint eid = batchEmitterList[batch.emitterListOffset + i];
            uint count = uint(float(simulationAliveCount[eid]) * frameConsts[eid].spawnRatio);

            // Global write offset for each emitter (used by BuildAliveIndicesCS)
            emitterWriteOffsets[batch.emitterListOffset + i] = globalOffset + totalInstances;

            totalInstances += count;
        }
        
        // drawArgs  ä
        uint argsIdx = batchID * 5;
        batchBillboardArgs[argsIdx + 0] = batch.indexCount;
        batchBillboardArgs[argsIdx + 1] = totalInstances;
        batchBillboardArgs[argsIdx + 2] = batch.startIndexLocation;
        batchBillboardArgs[argsIdx + 3] = batch.baseVertexLocation;
        batchBillboardArgs[argsIdx + 4] = 0;

        BatchSortParam sp;
        sp.baseOffset = globalOffset;
        sp.particleCount = totalInstances;
        batchSortParams[batchID] = sp;

        globalOffset += totalInstances;
    }
}
