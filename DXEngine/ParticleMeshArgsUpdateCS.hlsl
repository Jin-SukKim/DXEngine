// [ParticleMeshArgsUpdateCS.hlsl]
#include "ParticleCommon.hlsli"

StructuredBuffer<uint> activeCount : register(t0);
RWBuffer<uint> drawArgs : register(u0);

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x >= consts[emitterID].render.numMeshes) return;

    // 각 Emitter의 시작 offset (emitterID * 5) + Mesh별 offset (DTid.x * 5)
    // InstanceCount는 각 DrawIndexedInstancedArgs의 2번째 요소 (offset 1)
    uint baseOffset = emitterID * 5;  // emitter별 offset 추가
    uint index = baseOffset + DTid.x * 5 + 1;
    
    drawArgs[index] = activeCount[emitterID];
}