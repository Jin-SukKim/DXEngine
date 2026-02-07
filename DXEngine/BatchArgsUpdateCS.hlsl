#include "ParticleCommon.hlsli"

// 입력: 배치별 emitter 정보 (CPU에서 업로드)
StructuredBuffer<BatchEmitterInfo> batchInputInfo : register(t0);  // CPU가 globalEmitterID, readParticleOffset만 채움
StructuredBuffer<uint> batchOffsets : register(t1);  // 각 배치의 batchInputInfo 시작 인덱스
StructuredBuffer<uint> batchCounts : register(t2);   // 각 배치의 emitter 수

// 출력
RWStructuredBuffer<BatchEmitterInfo> batchOutputInfo : register(u0);  // particleCount, batchVertexStart 계산됨
RWBuffer<uint> batchBillboardArgs : register(u1);    // DrawInstancedArgs (4 uints per batch)
RWBuffer<uint> batchMeshArgs : register(u2);         // DrawIndexedInstancedArgs (5 uints per batch)

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint batchIdx = DTid.x;
    uint offset = batchOffsets[batchIdx];
    uint count = batchCounts[batchIdx];

    uint totalVertices = 0;
    for (uint i = 0; i < count; i++)
    {
        BatchEmitterInfo info = batchInputInfo[offset + i];
        uint eid = info.globalEmitterID;
        uint pCount = readCount[eid];

        batchOutputInfo[offset + i].globalEmitterID = eid;
        batchOutputInfo[offset + i].readParticleOffset = info.readParticleOffset;
        batchOutputInfo[offset + i].particleCount = pCount;
        batchOutputInfo[offset + i].batchVertexStart = totalVertices;

        totalVertices += pCount;
    }

    // Billboard DrawInstancedArgs: { vertexCount, instanceCount=1, startVertex=0, startInstance=0 }
    uint billboardIdx = batchIdx * 4;
    batchBillboardArgs[billboardIdx + 0] = totalVertices;
    batchBillboardArgs[billboardIdx + 1] = 1;
    batchBillboardArgs[billboardIdx + 2] = 0;
    batchBillboardArgs[billboardIdx + 3] = 0;

    // Mesh DrawIndexedInstancedArgs: { indexCount(CPU에서 설정), instanceCount, startIndex=0, baseVertex=0, startInstance=0 }
    uint meshIdx = batchIdx * 5;
    // indexCount는 CPU에서 미리 설정 (같은 모델의 모든 emitter가 동일)
    batchMeshArgs[meshIdx + 1] = totalVertices;  // instanceCount = 총 파티클 수
    batchMeshArgs[meshIdx + 2] = 0;
    batchMeshArgs[meshIdx + 3] = 0;
    batchMeshArgs[meshIdx + 4] = 0;
}
