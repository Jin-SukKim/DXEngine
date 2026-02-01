#include "ParticleCommon.hlsli"

// 바인딩 슬롯 (C++ 코드와 맞춰야 함)
StructuredBuffer<uint> activeCount : register(t0);
RWBuffer<uint> dispatchArgs : register(u0); // Update용 (DispatchIndirect)
RWBuffer<uint> billboardArgs : register(u1); // Render용 (DrawInstancedIndirect) <--- 추가!

[numthreads(256, 1, 1)] // 1,1,1은 비효율적이므로 256 권장
void main(uint3 DTid : SV_DispatchThreadID)
{
    // [수정 1] emitterID(상수) 대신 스레드 ID 사용
    uint myEmitterID = DTid.x;

    // 현재 Emitter의 파티클 개수 가져오기
    uint count = activeCount[myEmitterID];

    // --------------------------------------------------------
    // 1. Dispatch Args 갱신 (Update 단계용)
    // --------------------------------------------------------
    // uint3 구조체이므로 stride = 3
    uint dispatchIdx = myEmitterID * 3;
    
    // ParticleCS의 [numthreads]가 (256, 1, 1)이라면 256으로 나눔 (1024라면 1024)
    dispatchArgs[dispatchIdx + 0] = (count + 255) / 256;
    dispatchArgs[dispatchIdx + 1] = 1;
    dispatchArgs[dispatchIdx + 2] = 1;

    // --------------------------------------------------------
    // 2. Billboard Args 갱신 (Render 단계용) -> [이게 없어서 1개만 나옴]
    // --------------------------------------------------------
    // DrawInstancedArgs 구조체: { VertexCount, InstanceCount, StartVertex, StartInstance }
    // uint4 구조체이므로 stride = 4
    uint drawIdx = myEmitterID * 4;

    // Index 0: VertexCountPerInstance (초기값 유지하거나 여기서 설정 가능)
    // billboardArgs[drawIdx + 0] = 4; // Quad라면 4, 6이면 6

    // Index 1: InstanceCount (여기에 파티클 개수를 넣어야 다 그려짐!)
    billboardArgs[drawIdx + 0] = count;
    billboardArgs[drawIdx + 1] = 1;

    // Index 2, 3: StartVertex, StartInstance (보통 0)
    billboardArgs[drawIdx + 2] = 0;
    billboardArgs[drawIdx + 3] = 0;
}