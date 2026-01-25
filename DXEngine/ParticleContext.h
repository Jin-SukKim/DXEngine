#pragma once
#include "pch.h"
#include "Particle.h"
#include "AppendBuffer.h"
#include "IndirectArgsBuffer.h"
#include "MeshData.h"
#include "BitonicSort.h"

namespace DE {

    class RenderModule;
    class MaterialModule;

struct ParticleInitContext {
    ID3D11Device* device;
    ParticleConsts& consts;
    ParticleFrameConsts& frameConsts;
};

// Base Context (공통 정보)
struct ParticleContext {
    ID3D11DeviceContext* context;
    ConstantBuffer<ParticleConsts>& constBuffer;
    ConstantBuffer<ParticleFrameConsts>& frameConstBuffer;
};

// Simulation 단계
struct SimulationContext : ParticleContext {
    float dt;
    float time;
    AppendBuffer<Particle>& consumeBuffer;
    AppendBuffer<Particle>& appendBuffer;
    ID3D11ShaderResourceView* countSRV;
    ID3D11Buffer* dispatchArgs;
    ID3D11Device* device;
    RenderModule* renderModule;
    
    // Baked Spawn Position 데이터 (Texture Spawn용)
    StructuredBuffer<Vector3>* bakedSpawnPos = nullptr;
    UINT bakedCount = 0;
    
    // Render 관련 버퍼 (Render Module이 사용)
    BitonicSort* sortBuffer = nullptr;
    IndirectArgsBuffer<DrawInstancedArgs>* billboardArgsBuffer = nullptr;
    IndirectArgsBuffer<DrawIndexedInstancedArgs>* meshArgsBuffer = nullptr;
};

// Render 단계
struct RenderContext : ParticleContext {
    ID3D11ShaderResourceView* particleSRV;
    MaterialModule* materialModule;
    
    // Render 관련 버퍼
    BitonicSort* sortBuffer = nullptr;
    IndirectArgsBuffer<DrawInstancedArgs>* billboardArgsBuffer = nullptr;
    IndirectArgsBuffer<DrawIndexedInstancedArgs>* meshArgsBuffer = nullptr;
};

} // namespace DE