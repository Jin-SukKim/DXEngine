// [ParticleMeshArgsUpdateCS.hlsl]
#include "ParticleCommon.hlsli"
// 설명: 현재 활성화된 파티클 개수를 읽어, Indirect Draw Buffer의 InstanceCount를 갱신합니다.

// t0: 현재 살아있는 파티클 개수가 담긴 버퍼 (SRV)
Buffer<uint> activeCount : register(t0);

// u0: 갱신할 DrawIndexedInstancedArgs 버퍼 (UAV)
// 구조: { IndexCount, InstanceCount, StartIndex, BaseVertex, StartInstance }
RWBuffer<uint> drawArgs : register(u0);

[numthreads(256, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x >= render.numMeshes) return;

    // 각 Mesh 데이터의 IndirectAgs의 간격(Stride)은 5 uint이고
    // InstanceCount는 2번째 요소(offset 1)
    uint index = DTid.x * 5 + 1;
    // Draw Args의 두 번째 값(Offset 1)이 InstanceCount
    drawArgs[index] = activeCount[0];
}