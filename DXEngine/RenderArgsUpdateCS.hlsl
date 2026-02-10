#include "ParticleCommon.hlsli"

// ε  (C++ ڵ  )
RWBuffer<uint> billboardArgs : register(u0); // Render (DrawInstancedIndirect) <--- ߰!
RWBuffer<uint> meshArgs : register(u1);

[numthreads(256, 1, 1)] // 1,1,1 ȿ̹Ƿ 256 
void main(uint3 DTid : SV_DispatchThreadID)
{
    // [ 1] emitterID()   ID 
    uint myEmitterID = DTid.x;

    //  Emitter ƼŬ  
    uint count = readCount[myEmitterID];

    // Apply spawn ratio for distance-based overdraw control
    float ratio = frameConsts[myEmitterID].spawnRatio;
    uint adjustedCount = uint(float(count) * ratio);

    EmitterID eID = emitterIDs[myEmitterID];
    // --------------------------------------------------------
    // 2. Billboard Args  (Render ܰ) -> [̰  1 ]
    // --------------------------------------------------------
    // DrawInstancedArgs ü: { VertexCount, InstanceCount, StartVertex, StartInstance }
    // uint4 ü̹Ƿ stride = 4
    uint drawIdx = myEmitterID * 5; // DrawIndexedInstancedArgs는 5개 필드

    // DrawIndexedInstancedArgs: { IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation }
    billboardArgs[drawIdx + 0] = eID.indexCount; // 쿼드 메쉬는 6개 인덱스
    billboardArgs[drawIdx + 1] = adjustedCount; // 파티클 개수 (spawn ratio 적용)
    billboardArgs[drawIdx + 2] = eID.startIndexLocation; // StartIndexLocation
    billboardArgs[drawIdx + 3] = eID.baseVertexLocation; // BaseVertexLocation
    billboardArgs[drawIdx + 4] = 0; // StartInstanceLocation

    uint meshIdx = myEmitterID * 5;

    meshArgs[meshIdx] = consts[myEmitterID].render.indexCount;
    meshArgs[meshIdx + 1] = adjustedCount; // 파티클 개수 (spawn ratio 적용)
}