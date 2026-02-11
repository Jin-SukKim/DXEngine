#include "ParticleCommon.hlsli"

StructuredBuffer<uint> emitterWriteOffsets : register(t0);

RWStructuredBuffer<uint> aliveIndices : register(u0);

cbuffer BuildAliveConsts : register(b0) {
    uint numFlatEmitters;
    uint3 buildAlivePadding;
};

[numthreads(256, 1, 1)]
void main(uint3 groupID : SV_GroupID, uint3 threadID : SV_GroupThreadID)
{
    uint flatEmitterIdx = groupID.x;
    if (flatEmitterIdx >= numFlatEmitters) return;

    uint eid = batchEmitterList[flatEmitterIdx];
    uint count = uint(float(readCount[eid]) * frameConsts[eid].spawnRatio);
    uint writeBase = emitterWriteOffsets[flatEmitterIdx];
    uint readBase = emitterIDs[eid].readParticleOffset;

    // Each thread writes its assigned particle's global index
    for (uint i = threadID.x; i < count; i += 256) {
        aliveIndices[writeBase + i] = readBase + i;
    }
}
