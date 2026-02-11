#include "ParticleCommon.hlsli"

RWBuffer<uint> batchBillboardArgs : register(u0);

cbuffer BatchRenderArgsConsts : register(b0) {
    uint numBatches;
    uint3 batchArgsPadding;
};

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint batchID = DTid.x;
    if (batchID >= numBatches) return;

    BatchDescriptor batch = batchDescriptors[batchID];

    // Sum instance counts for all emitters in batch
    uint totalInstances = 0;
    for (uint i = 0; i < batch.emitterCount; i++) {
        uint eid = batchEmitterList[batch.emitterListOffset + i];
        uint count = uint(float(readCount[eid]) * frameConsts[eid].spawnRatio);
        totalInstances += count;
    }

    // Write merged draw args
    uint argsIdx = batchID * 5;
    batchBillboardArgs[argsIdx + 0] = 6;               // Index count (quad)
    batchBillboardArgs[argsIdx + 1] = totalInstances;  // Instance count
    batchBillboardArgs[argsIdx + 2] = 0;
    batchBillboardArgs[argsIdx + 3] = 0;
    batchBillboardArgs[argsIdx + 4] = 0;
}
