#include "ParticleCommon.hlsli"

RWBuffer<uint> batchBillboardArgs : register(u0);
RWStructuredBuffer<uint> emitterWriteOffsets : register(u1);

StructuredBuffer<uint> simulationAliveCount : register(t29);

cbuffer BatchRenderArgsConsts : register(b0) {
    uint numBatches;
    uint3 batchArgsPadding;
};

// 현재는 모든 Batch의 drawArgs를 계산 (Single Thread)
// TODO: Multi-Thread로 고칠 수 있을지 고민
[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint globalOffset = 0;

    for (uint batchID = 0; batchID < numBatches; batchID++) {
        // Batch
        BatchDescriptor batch = batchDescriptors[batchID];

        // Batch 단위로 Emitter의 id를 모으기 (이걸 사용해 AliveIndices를 채움)
        uint totalInstances = 0;
        for (uint i = 0; i < batch.emitterCount; i++) {
            uint eid = batchEmitterList[batch.emitterListOffset + i];
            uint count = uint(float(simulationAliveCount[eid]) * frameConsts[eid].spawnRatio);

            // Global write offset for each emitter (used by BuildAliveIndicesCS)
            emitterWriteOffsets[batch.emitterListOffset + i] = globalOffset + totalInstances;

            totalInstances += count;
        }
        
        // drawArgs 내용 채우기
        uint argsIdx = batchID * 5;
        batchBillboardArgs[argsIdx + 0] = batch.indexCount;
        batchBillboardArgs[argsIdx + 1] = totalInstances;
        batchBillboardArgs[argsIdx + 2] = batch.startIndexLocation;
        batchBillboardArgs[argsIdx + 3] = batch.baseVertexLocation;
        batchBillboardArgs[argsIdx + 4] = 0;

        globalOffset += totalInstances;
    }
}
