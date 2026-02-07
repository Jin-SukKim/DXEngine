#include "ParticleCommon.hlsli"

// 바인딩 슬롯 (C++ 코드와 맞춰야 함)
RWBuffer<uint> dispatchArgs : register(u0); // Update용 (DispatchIndirect)

[numthreads(256, 1, 1)] // 1,1,1은 비효율적이므로 256 권장
void main(uint3 DTid : SV_DispatchThreadID)
{
    // [수정 1] emitterID(상수) 대신 스레드 ID 사용
    uint myEmitterID = DTid.x;

    // 현재 Emitter의 파티클 개수 가져오기
    uint count = readCount[myEmitterID];

    // --------------------------------------------------------
    // 1. Dispatch Args 갱신 (Update 단계용)
    // --------------------------------------------------------
    // uint3 구조체이므로 stride = 3
    uint dispatchIdx = myEmitterID * 3;
    
    // ParticleCS의 [numthreads]가 (256, 1, 1)이라면 256으로 나눔 (1024라면 1024)
    dispatchArgs[dispatchIdx + 0] = (count + 255) / 256;
    dispatchArgs[dispatchIdx + 1] = 1;
    dispatchArgs[dispatchIdx + 2] = 1;
}