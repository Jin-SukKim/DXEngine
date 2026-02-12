#include "ParticleCommon.hlsli"

// Material 별로 모아둔 EmitterID의 index 배열
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

    // EmitterID의 index 가져오기
    uint eid = batchEmitterList[flatEmitterIdx];
    // 가져온 index값을 사용해 값 읽어오기
    uint count = uint(float(readCount[eid]) * frameConsts[eid].spawnRatio);
    uint writeBase = emitterWriteOffsets[flatEmitterIdx];
    uint readBase = emitterIDs[eid].readParticleOffset;

    // 각 Emitter의 Particle의 index 저장
    for (uint i = threadID.x; i < count; i += 256) {
        aliveIndices[writeBase + i] = readBase + i;
    }
}
