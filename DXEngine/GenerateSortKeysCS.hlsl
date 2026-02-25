#include "Common.hlsli"
#include "ParticleCommon.hlsli"

StructuredBuffer<uint> batchAliveIndices : register(t0);
StructuredBuffer<BatchSortParam> batchParams : register(t1); // 추가: Batch 경계 정보
RWStructuredBuffer<SortElement> sortBuffer : register(u0);

cbuffer SortParams : register(b5) {
    uint sortBaseOffset;
    uint sortParticleCount;
    uint firstBatchIdx;
    uint lastBatchIdx;
    float3 cameraForward;
    float pad1;
};

// Float를 크기 비교 가능한 uint로 변환하는 헬퍼 함수
uint FloatToSortableUint(float f) {
    uint u = asuint(f);
    return (u & 0x80000000) ? ~u : (u | 0x80000000);
}

[numthreads(1024, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint id = dtID.x;
    uint baseOffset = sortBaseOffset;
    uint particleCount = sortParticleCount;

    SortElement elem;
    if (id < particleCount)
    {
        uint gIdx = baseOffset + id;
        uint particleIdx = batchAliveIndices[gIdx];
        Particle p = readParticles[particleIdx];

        float3 toParticle = p.position - eyeWorld;
        float viewZ = dot(toParticle, cameraForward);

        // 1. 현재 파티클이 속한 Batch Index 찾기 (AlphaBlend Batch 개수는 적으므로 단순 루프 무방)
        uint batchIdx = firstBatchIdx;
        for (uint b = firstBatchIdx; b <= lastBatchIdx; ++b) {
            if (gIdx >= batchParams[b].baseOffset &&
                gIdx < batchParams[b].baseOffset + batchParams[b].particleCount) {
                batchIdx = b;
                break;
            }
        }

        // 2. Key 할당 (내림차순 정렬 기준)
        // BatchID가 작을수록(먼저 그릴 Batch) 앞에 와야 하므로 반전시킴
        elem.key.x = 0xFFFFFFFF - batchIdx;
        // viewZ가 클수록(멀리 있을수록) 앞에 와야 하므로 (Back-To-Front) Float 그대로 Sortable 변환
        elem.key.y = FloatToSortableUint(viewZ);
        elem.value = particleIdx;
    }
    else
    {
        // 쓰레기 값은 정렬 시 맨 뒤로 밀리도록 최소값 할당
        elem.key.x = 0;
        elem.key.y = 0;
        elem.value = 0xFFFFFFFF;
    }

    sortBuffer[id] = elem;
}