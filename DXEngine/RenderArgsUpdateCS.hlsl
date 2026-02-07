#include "ParticleCommon.hlsli"

// 바인딩 슬롯 (C++ 코드와 맞춰야 함)
RWBuffer<uint> billboardArgs : register(u0); // Render용 (DrawInstancedIndirect) <--- 추가!
RWBuffer<uint> meshArgs : register(u1);

[numthreads(256, 1, 1)] // 1,1,1은 비효율적이므로 256 권장
void main(uint3 DTid : SV_DispatchThreadID)
{
    // [수정 1] emitterID(상수) 대신 스레드 ID 사용
    uint myEmitterID = DTid.x;

    // 현재 Emitter의 파티클 개수 가져오기
    uint count = readCount[myEmitterID];

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

    uint meshIdx = myEmitterID * 5;

    meshArgs[meshIdx] = consts[myEmitterID].render.indexCount;
    meshArgs[meshIdx + 1] = count;
}