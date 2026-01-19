#include "ParticleCommon.hlsli"

Buffer<uint> particleCountBuffer : register(t0);
RWBuffer<uint> dispatchArgs : register(u0);
RWBuffer<uint> drawArgs : register(u1);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint count = particleCountBuffer[0];

    dispatchArgs[0] = (count + 255) / 256;
    dispatchArgs[1] = 1;
    dispatchArgs[2] = 1;

    if (render.meshIndexCount > 0) {
        // Mesh Rendering: DrawIndexedInstancedIndirect
        // 구조: [IndexCount, InstanceCount, StartIndex, BaseVertex, StartInstance] x N
        // RWBuffer<uint>로 접근하므로 stride는 5

        // CPU에서 이미 IndexCount 등 정적인 데이터는 채워넣었으므로,
        // 여기서는 InstanceCount(파티클 개수)만 업데이트하면 됩니다.
        for (uint i = 0; i < render.meshCount; ++i)
        {
            uint offset = i * 5;
            // drawArgs[offset + 0] = IndexCount; // (CPU 값 유지)
            drawArgs[offset + 1] = count;         // InstanceCount 업데이트
            // drawArgs[offset + 2] = StartIndex; // (CPU 값 유지)
            // drawArgs[offset + 3] = BaseVertex; // (CPU 값 유지)
            // drawArgs[offset + 4] = StartInstance; // (CPU 값 유지)
        }
    }
    else {
        drawArgs[0] = count;
        drawArgs[1] = 1;
        drawArgs[2] = 0;
        drawArgs[3] = 0;
        drawArgs[4] = 0;
    }
    
}